#include "stdafx.h"
#include "RayTracer.h"
#include "HonoursApplication.h"
#include "Utilities.h"
#include "UploadBuffer.h"
#include <dxcapi.h>
#include <fstream>
#include <sstream>

const wchar_t* RayTracer::hit_group_name_ = L"HitGroup";
const wchar_t* RayTracer::ray_gen_shader_name_ = L"RayGenerationShader";
const wchar_t* RayTracer::intersection_shader_name_ = L"IntersectionShader";
const wchar_t* RayTracer::closest_hit_shader_name_ = L"ClosestHitShader";
const wchar_t* RayTracer::miss_shader_name = L"MissShader";

RayTracer::RayTracer(DX::DeviceResources* device_resources, HonoursApplication* app, Computer* comp) :
    device_resources_(device_resources), application_(app), computer_(comp)
{
    device_resources_->GetCommandList()->Reset(device_resources_->GetCommandAllocator(), nullptr);

    acceleration_structure_ = std::make_unique<AccelerationStructureManager>(device_resources);

    CreateRootSignatures();
    CreateRaytracingPipelineStateObject();
    BuildAccelerationStructures();

    device_resources_->GetCommandList()->Reset(device_resources_->GetCommandAllocator(), nullptr);   // only need when using naive aabb
    BuildShaderTables();
    CreateRaytracingOutputResource();

    // Execute command list and wait until assets have been uploaded to the GPU.
    device_resources_->ExecuteCommandList();
    device_resources_->WaitForGpu();
}

void RayTracer::RayTracing()
{
    auto commandList = device_resources_->GetCommandList();

    commandList->SetComputeRootSignature(rt_global_root_signature_.Get());

    // Bind the heaps, acceleration structure and dispatch rays.
    ID3D12DescriptorHeap* heap = application_->GetDescriptorHeap();
    commandList->SetDescriptorHeaps(1, &heap);
    commandList->SetComputeRootDescriptorTable(GlobalRTRootSignatureParams::OutputViewSlot, m_raytracingOutputResourceUAVGpuDescriptor);
    commandList->SetComputeRootDescriptorTable(GlobalRTRootSignatureParams::SDFTextureSlot, computer_->GetSDFTextureHandle());
    //if (acceleration_structure_->IsStructureBuilt()) {
    //    //OutputDebugString(L"\nSTRUCTURE BUILT\n:");
    //    commandList->SetComputeRootShaderResourceView(GlobalRTRootSignatureParams::AccelerationStructureSlot, acceleration_structure_->GetTLAS()->GetGPUVirtualAddress());
    //}
    //else {
        //OutputDebugString(L"\nSTRUCTURE NOT BUILT\n:");
        commandList->SetComputeRootShaderResourceView(GlobalRTRootSignatureParams::AccelerationStructureSlot, m_topLevelAccelerationStructure->GetGPUVirtualAddress());
    //}

    commandList->SetComputeRootShaderResourceView(GlobalRTRootSignatureParams::ParticlePositionsBufferSlot, computer_->GetPositionsBuffer()->GetGPUVirtualAddress());
    commandList->SetComputeRootConstantBufferView(GlobalRTRootSignatureParams::ConstantBufferSlot, application_->GetRaytracingCB()->GetGPUVirtualAddress());

    // Since each shader table has only one shader record, the stride is same as the size.
    D3D12_DISPATCH_RAYS_DESC dispatchDesc = {};
    dispatchDesc.HitGroupTable.StartAddress = m_hitGroupShaderTable->Resource()->GetGPUVirtualAddress();
    dispatchDesc.HitGroupTable.SizeInBytes = m_hitGroupShaderTable->Resource()->GetDesc().Width;
    dispatchDesc.HitGroupTable.StrideInBytes = dispatchDesc.HitGroupTable.SizeInBytes;
    dispatchDesc.MissShaderTable.StartAddress = m_missShaderTable->Resource()->GetGPUVirtualAddress();
    dispatchDesc.MissShaderTable.SizeInBytes = m_missShaderTable->Resource()->GetDesc().Width;
    dispatchDesc.MissShaderTable.StrideInBytes = dispatchDesc.MissShaderTable.SizeInBytes;
    dispatchDesc.RayGenerationShaderRecord.StartAddress = m_rayGenShaderTable->Resource()->GetGPUVirtualAddress();
    dispatchDesc.RayGenerationShaderRecord.SizeInBytes = m_rayGenShaderTable->Resource()->GetDesc().Width;
    dispatchDesc.Width = window_size_.x;
    dispatchDesc.Height = window_size_.y;
    dispatchDesc.Depth = 1;
    commandList->SetPipelineState1(rt_state_object_.Get());

    /*commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(acceleration_structure_->GetBLAS()));
    commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(acceleration_structure_->GetTLAS()));*/
    commandList->DispatchRays(&dispatchDesc);
    //commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(acceleration_structure_->GetBLAS()));
    //commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(acceleration_structure_->GetTLAS()));


    commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(computer_->GetPositionsBuffer(),D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
    if (!(application_->GetDebugValues().render_analytical_ || application_->GetDebugValues().visualize_particles_)) {
        commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(computer_->GetSDFTexture(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
    }
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
    // ---- Global Root Signature ----

    // (u)
    CD3DX12_DESCRIPTOR_RANGE UAVDescriptor;
    UAVDescriptor.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
    CD3DX12_ROOT_PARAMETER rootParameters[GlobalRTRootSignatureParams::Count];
    rootParameters[GlobalRTRootSignatureParams::OutputViewSlot].InitAsDescriptorTable(1, &UAVDescriptor);

    // (t)
    rootParameters[GlobalRTRootSignatureParams::AccelerationStructureSlot].InitAsShaderResourceView(0);
    rootParameters[GlobalRTRootSignatureParams::ParticlePositionsBufferSlot].InitAsShaderResourceView(1);
    CD3DX12_DESCRIPTOR_RANGE tex_descriptor;
    tex_descriptor.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2);
    rootParameters[GlobalRTRootSignatureParams::SDFTextureSlot].InitAsDescriptorTable(1, &tex_descriptor);

    // (b)
    rootParameters[GlobalRTRootSignatureParams::ConstantBufferSlot].InitAsConstantBufferView(0);

    // Sampler
    CD3DX12_STATIC_SAMPLER_DESC sampler(0, D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

    CD3DX12_ROOT_SIGNATURE_DESC globalRootSignatureDesc(ARRAYSIZE(rootParameters), rootParameters, 1, &sampler);
    SerializeAndCreateRaytracingRootSignature(globalRootSignatureDesc, &rt_global_root_signature_);


    // ---- Hit group local root signature ----

    // (t)
    CD3DX12_ROOT_PARAMETER local_root_params[LocalRootSignatureParams::Count];
    local_root_params[LocalRootSignatureParams::AABBBufferSlot].InitAsShaderResourceView(0, 1);
    CD3DX12_ROOT_SIGNATURE_DESC localRootSignatureDesc(ARRAYSIZE(local_root_params), local_root_params, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);
    SerializeAndCreateRaytracingRootSignature(localRootSignatureDesc, &rt_hit_local_root_signature_);

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

    CD3DX12_STATE_OBJECT_DESC raytracingPipeline{ D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE };

    // DXIL library
    // This contains the shaders and their entrypoints for the state object.
    // Since shaders are not considered a subobject, they need to be passed in via DXIL library subobjects.
    auto lib = raytracingPipeline.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
    //ComPtr<ID3DBlob> blob;
    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION; // allow debugging !!!!!!!!!!!!!! - prob dont do this in final build

    auto blob = CompileShaderLibrary(application_->GetAssetFullPath(L"RayTracing.hlsl").c_str());
    //ThrowIfFailed(D3DCompileFromFile(L"assets/RayTracing.hlsl", NULL, NULL, NULL, "lib", compileFlags, 0, &blob, nullptr));
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

    // Create and associate the local root signature with the hit group
    const auto localRootSig = raytracingPipeline.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
    localRootSig->SetRootSignature(rt_hit_local_root_signature_.Get());

    const auto localRootSigAssociation = raytracingPipeline.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
    localRootSigAssociation->SetSubobjectToAssociate(*localRootSig);
    localRootSigAssociation->AddExport(hit_group_name_);

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

    // Create and upload the AABBs
    D3D12_RAYTRACING_AABB aabb[8];
    aabb[0].MaxX = 1.f;
    aabb[0].MaxY = 1.f;
    aabb[0].MaxZ = 1.f;
    aabb[0].MinX = 0.f;
    aabb[0].MinY = 0.f;
    aabb[0].MinZ = 0.f;

    aabb[1].MaxX = 2.f;
    aabb[1].MaxY = 1.f;
    aabb[1].MaxZ = 1.f;
    aabb[1].MinX = 1.f;
    aabb[1].MinY = 0.f;
    aabb[1].MinZ = 0.f;

    aabb[2].MaxX = 1.f;
    aabb[2].MaxY = 1.f;
    aabb[2].MaxZ = 2.f;
    aabb[2].MinX = 0.f;
    aabb[2].MinY = 0.f;
    aabb[2].MinZ = 1.f;

    aabb[3].MaxX = 2.f;
    aabb[3].MaxY = 1.f;
    aabb[3].MaxZ = 2.f;
    aabb[3].MinX = 1.f;
    aabb[3].MinY = 0.f;
    aabb[3].MinZ = 1.f;

    aabb[4].MaxX = 1.f;
    aabb[4].MaxY = 2.f;
    aabb[4].MaxZ = 1.f;
    aabb[4].MinX = 0.f;
    aabb[4].MinY = 1.f;
    aabb[4].MinZ = 0.f;

    aabb[5].MaxX = 2.f;
    aabb[5].MaxY = 2.f;
    aabb[5].MaxZ = 1.f;
    aabb[5].MinX = 1.f;
    aabb[5].MinY = 1.f;
    aabb[5].MinZ = 0.f;

    aabb[6].MaxX = 1.f;
    aabb[6].MaxY = 2.f;
    aabb[6].MaxZ = 2.f;
    aabb[6].MinX = 0.f;
    aabb[6].MinY = 1.f;
    aabb[6].MinZ = 1.f;

    aabb[7].MaxX = 2.f;
    aabb[7].MaxY = 2.f;
    aabb[7].MaxZ = 2.f;
    aabb[7].MinX = 1.f;
    aabb[7].MinY = 1.f;
    aabb[7].MinZ = 1.f;

    aabb_buffer_ = Utilities::CreateDefaultBuffer(device, command_list, &aabb, /*(size of aabb array * )*/ sizeof(aabb), aabb_buffer_uploader_);
    aabb_buffer_.Get()->SetName(L"AABB buffer");
    aabb_buffer_uploader_->SetName(L"AABB uploader");

    // Build geometry description
    D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = {};
    geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS;
    geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE; // change later
    geometryDesc.AABBs.AABBCount = 8;
    geometryDesc.AABBs.AABBs.StartAddress = aabb_buffer_->GetGPUVirtualAddress();
    geometryDesc.AABBs.AABBs.StrideInBytes = sizeof(D3D12_RAYTRACING_AABB);

    // Get required sizes for an acceleration structure.
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE; // will change this later
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
    Utilities::AllocateDefaultBuffer(device, max(topLevelPrebuildInfo.ScratchDataSizeInBytes, bottomLevelPrebuildInfo.ScratchDataSizeInBytes), &scratchResource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

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
    //ComPtr<ID3D12Resource> instanceDescs;
    D3D12_RAYTRACING_INSTANCE_DESC instanceDesc = {};
    instanceDesc.Transform[0][0] = instanceDesc.Transform[1][1] = instanceDesc.Transform[2][2] = 1;
    instanceDesc.InstanceMask = 1;
    instanceDesc.AccelerationStructure = m_bottomLevelAccelerationStructure->GetGPUVirtualAddress();
    UploadBuffer<D3D12_RAYTRACING_INSTANCE_DESC> instance_desc_buffer(device, 1, false);
    instance_desc_buffer.CopyData(0, instanceDesc);

    //AllocateUploadBuffer(device, &instanceDesc, sizeof(instanceDesc), &instanceDescs, L"InstanceDescs");

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

    ReleaseUploaders();
}

// Build shader tables.
// This encapsulates all shader records - shaders and the arguments for their local root signatures.
void RayTracer::BuildShaderTables()
{
    auto device = device_resources_->GetD3DDevice();

    void* rayGenShaderIdentifier;
    void* missShaderIdentifier;
    void* hitGroupShaderIdentifier;

    auto GetShaderIdentifiers = [&](auto* stateObjectProperties)
        {
            rayGenShaderIdentifier = stateObjectProperties->GetShaderIdentifier(ray_gen_shader_name_);
            missShaderIdentifier = stateObjectProperties->GetShaderIdentifier(miss_shader_name);
            hitGroupShaderIdentifier = stateObjectProperties->GetShaderIdentifier(hit_group_name_);
        };

    // Get shader identifiers.
    UINT shaderIdentifierSize;
    {
        ComPtr<ID3D12StateObjectProperties> stateObjectProperties;
        ThrowIfFailed(rt_state_object_.As(&stateObjectProperties));
        GetShaderIdentifiers(stateObjectProperties.Get());
        shaderIdentifierSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    }

    // Ray gen shader table
    {
        UINT numShaderRecords = 1;
        UINT shaderRecordSize = shaderIdentifierSize;
        m_rayGenShaderTable = std::make_unique<ShaderTable>(device, numShaderRecords, shaderRecordSize);
        m_rayGenShaderTable->CopyRecord(0, ShaderRecord(rayGenShaderIdentifier));
    }

    // Miss shader table
    {
        UINT numShaderRecords = 1;
        UINT shaderRecordSize = shaderIdentifierSize;
        m_missShaderTable = std::make_unique<ShaderTable>(device, numShaderRecords, shaderRecordSize);
        m_missShaderTable->CopyRecord(0, ShaderRecord(missShaderIdentifier));
    }

    // Hit group shader table
    {
        UINT numShaderRecords = 1;
        UINT shaderRecordSize = shaderIdentifierSize;
        m_hitGroupShaderTable = std::make_unique<ShaderTable>(device, numShaderRecords, shaderRecordSize);
        m_hitGroupShaderTable->CopyRecord(0, ShaderRecord(hitGroupShaderIdentifier));
    }
}

// Create 2D output texture for raytracing.
void RayTracer::CreateRaytracingOutputResource()
{
    m_raytracingOutput.Reset();

    auto device = device_resources_->GetD3DDevice();
    auto backbufferFormat = device_resources_->GetBackBufferFormat();
    UINT descriptor_size = device_resources_->GetD3DDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    window_size_.x = device_resources_->GetOutputSize().right;
    window_size_.y = device_resources_->GetOutputSize().bottom;

    // Create the output resource. The dimensions and format should match the swap-chain.
    auto uavDesc = CD3DX12_RESOURCE_DESC::Tex2D(backbufferFormat, window_size_.x, window_size_.y, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    ThrowIfFailed(device->CreateCommittedResource(
        &defaultHeapProperties, 
        D3D12_HEAP_FLAG_NONE,
        &uavDesc,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        nullptr, IID_PPV_ARGS(&m_raytracingOutput)));

    D3D12_CPU_DESCRIPTOR_HANDLE uavDescriptorHandle;
    m_raytracingOutputResourceUAVDescriptorHeapIndex = application_->AllocateDescriptor(&uavDescriptorHandle, m_raytracingOutputResourceUAVDescriptorHeapIndex);
    

    D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
    UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    device->CreateUnorderedAccessView(m_raytracingOutput.Get(), nullptr, &UAVDesc, uavDescriptorHandle);

    m_raytracingOutputResourceUAVGpuDescriptor = CD3DX12_GPU_DESCRIPTOR_HANDLE(application_->GetDescriptorHeap()->GetGPUDescriptorHandleForHeapStart(), m_raytracingOutputResourceUAVDescriptorHeapIndex, descriptor_size);
}

// Compile a HLSL file into a DXIL library - from NVIDIA
// 
/*-----------------------------------------------------------------------
Copyright (c) 2014-2018, NVIDIA. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
* Neither the name of its contributors may be used to endorse
or promote products derived from this software without specific
prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-----------------------------------------------------------------------*/
//
IDxcBlob* RayTracer::CompileShaderLibrary(LPCWSTR fileName)
{
    static IDxcCompiler* pCompiler = nullptr;
    static IDxcLibrary* pLibrary = nullptr;
    static IDxcIncludeHandler* dxcIncludeHandler;

    HRESULT hr;

    // Initialize the DXC compiler and compiler helper
    if (!pCompiler)
    {
        ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler, __uuidof(IDxcCompiler), (void**)&pCompiler));
        ThrowIfFailed(DxcCreateInstance(CLSID_DxcLibrary, __uuidof(IDxcLibrary), (void**)&pLibrary));
        ThrowIfFailed(pLibrary->CreateIncludeHandler(&dxcIncludeHandler));
    }
    // Open and read the file
    std::ifstream shaderFile(fileName);
    if (shaderFile.good() == false)
    {
        throw std::logic_error("Cannot find shader file");
    }
    std::stringstream strStream;
    strStream << shaderFile.rdbuf();
    std::string sShader = strStream.str();

    // Create blob from the string
    IDxcBlobEncoding* pTextBlob;
    ThrowIfFailed(pLibrary->CreateBlobWithEncodingFromPinned(
        (LPBYTE)sShader.c_str(), (uint32_t)sShader.size(), 0, &pTextBlob));

    // Compile
    IDxcOperationResult* pResult;
    ThrowIfFailed(pCompiler->Compile(pTextBlob, fileName, L"", L"lib_6_3", nullptr, 0, nullptr, 0,
        dxcIncludeHandler, &pResult));

    // Verify the result
    HRESULT resultCode;
    ThrowIfFailed(pResult->GetStatus(&resultCode));
    if (FAILED(resultCode))
    {
        IDxcBlobEncoding* pError;
        hr = pResult->GetErrorBuffer(&pError);
        if (FAILED(hr))
        {
            throw std::logic_error("Failed to get shader compiler error");
        }

        // Convert error blob to a string
        std::vector<char> infoLog(pError->GetBufferSize() + 1);
        memcpy(infoLog.data(), pError->GetBufferPointer(), pError->GetBufferSize());
        infoLog[pError->GetBufferSize()] = 0;

        std::string errorMsg = "Shader Compiler Error:\n";
        errorMsg.append(infoLog.data());

        MessageBoxA(nullptr, errorMsg.c_str(), "Error!", MB_OK);
        throw std::logic_error("Failed compile shader");
    }

    IDxcBlob* pBlob;
    ThrowIfFailed(pResult->GetResult(&pBlob));
    return pBlob;
}