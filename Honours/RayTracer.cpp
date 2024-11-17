#include "stdafx.h"
#include "RayTracer.h"
#include "Utilities.h"
#include "UploadBuffer.h"

const wchar_t* RayTracer::hit_group_name_ = L"HitGroup";
const wchar_t* RayTracer::ray_gen_shader_name_ = L"RayGenerationShader";
const wchar_t* RayTracer::intersection_shader_name_ = L"IntersectionShader";
const wchar_t* RayTracer::closest_hit_shader_name_ = L"ClosestHitShader";
const wchar_t* RayTracer::miss_shader_name = L"MissShader";

RayTracer::RayTracer(DX::DeviceResources* device_resources) :
    device_resources_(device_resources)
{
    CreateRootSignatures();
    CreateRaytracingPipelineStateObject();
    BuildAccelerationStructures();
}

void RayTracer::CheckRayTracingSupport(ID3D12Device5* device)
{
    D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
    ThrowIfFailed(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5)));
    if (options5.RaytracingTier < D3D12_RAYTRACING_TIER_1_0) {
        throw std::runtime_error("Device does not support DXR raytracing!");
    }
}

void RayTracer::CreateRootSignatures()
{
    // Global Root Signature
    // This is a root signature that is shared across all raytracing shaders invoked during a DispatchRays() call.
    {
        CD3DX12_DESCRIPTOR_RANGE UAVDescriptor;
        UAVDescriptor.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
        CD3DX12_ROOT_PARAMETER rootParameters[GlobalRootSignatureParams::Count];
        rootParameters[GlobalRootSignatureParams::OutputViewSlot].InitAsDescriptorTable(1, &UAVDescriptor);
        rootParameters[GlobalRootSignatureParams::AccelerationStructureSlot].InitAsShaderResourceView(0);
        rootParameters[GlobalRootSignatureParams::ConstantBufferSlot].InitAsConstantBufferView(0);
        CD3DX12_ROOT_SIGNATURE_DESC globalRootSignatureDesc(ARRAYSIZE(rootParameters), rootParameters);
        SerializeAndCreateRaytracingRootSignature(globalRootSignatureDesc, &rt_global_root_signature_);
    }

    // Local Root Signature
    // This is a root signature that enables a shader to have unique arguments that come from shader tables.
    /*{
        CD3DX12_ROOT_PARAMETER rootParameters[LocalRootSignatureParams::Count];
        rootParameters[LocalRootSignatureParams::ViewportConstantSlot].InitAsConstants(SizeOfInUint32(m_rayGenCB), 0, 0);
        CD3DX12_ROOT_SIGNATURE_DESC localRootSignatureDesc(ARRAYSIZE(rootParameters), rootParameters);
        localRootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
        SerializeAndCreateRaytracingRootSignature(localRootSignatureDesc, &rt_local_root_signature_);
    }*/
}

void RayTracer::SerializeAndCreateRaytracingRootSignature(D3D12_ROOT_SIGNATURE_DESC& desc, ComPtr<ID3D12RootSignature>* rootSig)
{
    ComPtr<ID3DBlob> blob;
    ComPtr<ID3DBlob> error;

    ThrowIfFailed(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error), error ? static_cast<wchar_t*>(error->GetBufferPointer()) : nullptr);
    ThrowIfFailed(device_resources_->GetD3DDevice()->CreateRootSignature(1, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&(*rootSig))));
}

// Create a raytracing pipeline state object (RTPSO).
// An RTPSO represents a full set of shaders reachable by a DispatchRays() call,
// with all configuration options resolved, such as local signatures and other state.
void RayTracer::CreateRaytracingPipelineStateObject()
{
    // Create 7 subobjects that combine into a RTPSO:
    // 1 - DXIL library
    // 1 - Triangle hit group
    // 1 - Shader config
    // 2 - Local root signature and association - (not currently).
    // 1 - Global root signature
    // 1 - Pipeline config
    CD3DX12_STATE_OBJECT_DESC raytracingPipeline{ D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE };

    // DXIL library
    // This contains the shaders and their entrypoints for the state object.
    // Since shaders are not considered a subobject, they need to be passed in via DXIL library subobjects.
    auto lib = raytracingPipeline.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
    ComPtr<ID3DBlob> blob;
    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION; // allow debugging !!!!!!!!!!!!!! - prob dont do this in final build
    ThrowIfFailed(D3DCompileFromFile(L"assets/RayTracing.hlsl", nullptr, nullptr, "main", "lib", compileFlags, 0, &blob, nullptr));
   // ThrowIfFailed(D3DShaderCompiler::CompileFromFile(L"assets/shaders/raytracing/raytracing.hlsl", L"main", L"lib", {}, &blob));
    D3D12_SHADER_BYTECODE libdxil = CD3DX12_SHADER_BYTECODE(blob->GetBufferPointer(), blob->GetBufferSize());
    lib->SetDXILLibrary(&libdxil);

    // Hit group
    // Closest hit and intersection shaders to be executed when a ray intersects the geometry's AABB.
    const auto hitGroup = raytracingPipeline.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
    hitGroup->SetIntersectionShaderImport(intersection_shader_name_);
    hitGroup->SetClosestHitShaderImport(closest_hit_shader_name_);
    hitGroup->SetHitGroupExport(hit_group_name_);
    hitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE);

    // Shader config
    // Defines the maximum sizes in bytes for the ray payload and attribute structure.
    auto shaderConfig = raytracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
    UINT payloadSize = sizeof(RayPayload);   
    UINT attributeSize = sizeof(RayIntersectionAttributes); 
    shaderConfig->Config(payloadSize, attributeSize);

    // Global root signature
    // This is a root signature that is shared across all raytracing shaders invoked during a DispatchRays() call.
    auto globalRootSignature = raytracingPipeline.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
    globalRootSignature->SetRootSignature(rt_global_root_signature_.Get());

    // Pipeline config
    // Defines the maximum TraceRay() recursion depth.
    auto pipelineConfig = raytracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
    UINT maxRecursionDepth = MAX_RECURSION_DEPTH;
    pipelineConfig->Config(maxRecursionDepth);

    // Create the state object.
    ThrowIfFailed(device_resources_->GetD3DDevice()->CreateStateObject(raytracingPipeline, IID_PPV_ARGS(&rt_state_object_)));
}

// Build acceleration structures needed for raytracing.
void RayTracer::BuildAccelerationStructures()
{
    ID3D12Device5* device = device_resources_->GetD3DDevice();
    ID3D12GraphicsCommandList4* command_list = device_resources_->GetCommandList();

    // Create and upload the AABB
    D3D12_RAYTRACING_AABB aabb;
    aabb.MaxX = 1.f;
    aabb.MaxY = 1.f;
    aabb.MaxZ = 1.f;
    aabb.MinX = 0.f;
    aabb.MinY = 0.f;
    aabb.MinZ = 0.f;

    aabb_buffer_ = Utilities::CreateDefaultBuffer(device, command_list, &aabb, /*(size of aabb array * )*/ sizeof(D3D12_RAYTRACING_AABB), aabb_buffer_uploader_);

    // Build geometry description
    D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = {};
    geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS;
    geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
    geometryDesc.AABBs.AABBCount = 1;
    geometryDesc.AABBs.AABBs.StartAddress = aabb_buffer_->GetGPUVirtualAddress();
    geometryDesc.AABBs.AABBs.StrideInBytes = sizeof(D3D12_RAYTRACING_AABB);

    // Get required sizes for an acceleration structure.
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS topLevelInputs = {};
    topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    topLevelInputs.Flags = buildFlags;
    topLevelInputs.NumDescs = 1;
    topLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo = {};
    device->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs, &topLevelPrebuildInfo);
    //ThrowIfFalse(topLevelPrebuildInfo.ResultDataMaxSizeInBytes > 0);

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottomLevelPrebuildInfo = {};
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS bottomLevelInputs = topLevelInputs;
    bottomLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    bottomLevelInputs.pGeometryDescs = &geometryDesc;
    device->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelInputs, &bottomLevelPrebuildInfo);
    //ThrowIfFalse(bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes > 0);

    ComPtr<ID3D12Resource> scratchResource;
    Utilities::AllocateDefaultBuffer(device, max(topLevelPrebuildInfo.ScratchDataSizeInBytes, bottomLevelPrebuildInfo.ScratchDataSizeInBytes), &scratchResource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    // Allocate resources for acceleration structures.
    // Acceleration structures can only be placed in resources that are created in the default heap (or custom heap equivalent). 
    // Default heap is OK since the application doesn’t need CPU read/write access to them. 
    // The resources that will contain acceleration structures must be created in the state D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, 
    // and must have resource flag D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS. The ALLOW_UNORDERED_ACCESS requirement simply acknowledges both: 
    //  - the system will be doing this type of access in its implementation of acceleration structure builds behind the scenes.
    //  - from the app point of view, synchronization of writes/reads to acceleration structures is accomplished using UAV barriers.
    {
        D3D12_RESOURCE_STATES initialResourceState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;

        Utilities::AllocateDefaultBuffer(device, bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes, &m_bottomLevelAccelerationStructure, initialResourceState, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        Utilities::AllocateDefaultBuffer(device, topLevelPrebuildInfo.ResultDataMaxSizeInBytes, &m_topLevelAccelerationStructure, initialResourceState, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    }

    // Create an instance desc for the bottom-level acceleration structure.
   // ComPtr<ID3D12Resource> instanceDescs;
    D3D12_RAYTRACING_INSTANCE_DESC instanceDesc = {};
    instanceDesc.Transform[0][0] = instanceDesc.Transform[1][1] = instanceDesc.Transform[2][2] = 1;
    instanceDesc.InstanceMask = 1;
    instanceDesc.AccelerationStructure = m_bottomLevelAccelerationStructure->GetGPUVirtualAddress();
    UploadBuffer<D3D12_RAYTRACING_INSTANCE_DESC> instance_desc_buffer(device, 1, false);
    instance_desc_buffer.CopyData(0, instanceDesc);

   // AllocateUploadBuffer(device, &instanceDesc, sizeof(instanceDesc), &instanceDescs, L"InstanceDescs");

    // Bottom Level Acceleration Structure desc
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildDesc = {};
    {
        bottomLevelBuildDesc.Inputs = bottomLevelInputs;
        bottomLevelBuildDesc.ScratchAccelerationStructureData = scratchResource->GetGPUVirtualAddress();
        bottomLevelBuildDesc.DestAccelerationStructureData = m_bottomLevelAccelerationStructure->GetGPUVirtualAddress();
    }

    // Top Level Acceleration Structure desc
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelBuildDesc = {};
    {
        topLevelInputs.InstanceDescs = instance_desc_buffer.Resource()->GetGPUVirtualAddress();
        topLevelBuildDesc.Inputs = topLevelInputs;
        topLevelBuildDesc.DestAccelerationStructureData = m_topLevelAccelerationStructure->GetGPUVirtualAddress();
        topLevelBuildDesc.ScratchAccelerationStructureData = scratchResource->GetGPUVirtualAddress();
    }

    /*auto BuildAccelerationStructure = [&](auto* raytracingCommandList)
        {*/
            command_list->BuildRaytracingAccelerationStructure(&bottomLevelBuildDesc, 0, nullptr);
            command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(m_bottomLevelAccelerationStructure.Get()));
            command_list->BuildRaytracingAccelerationStructure(&topLevelBuildDesc, 0, nullptr);
        //};

    // Build acceleration structure.
    //BuildAccelerationStructure(command_list_);

    // Kick off acceleration structure construction.
    device_resources_->ExecuteCommandList();

    // Wait for GPU to finish as the locally created temporary GPU resources will get released once we go out of scope.
    device_resources_->WaitForGpu();
}