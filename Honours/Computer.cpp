#include "stdafx.h"
#include "Computer.h"
#include "HonoursApplication.h"

Computer::Computer(DX::DeviceResources* device_resources, HonoursApplication* app) :
	device_resources_(device_resources), application_(app)
{
    device_resources_->GetCommandList()->Reset(device_resources_->GetCommandAllocator(), nullptr);

	CreateRootSignatures();
	CreateComputePipelineStateObjects();
    CreateBuffers();
    CreateTexture3D();

    // Execute command list and wait until assets have been uploaded to the GPU.
    device_resources_->ExecuteCommandList();
    device_resources_->WaitForGpu();

    ReleaseUploaders();
}

void Computer::ComputePostitions()
{
    auto commandList = device_resources_->GetCommandList();

    commandList->SetPipelineState(compute_pos_state_object_.Get());
    commandList->SetComputeRootSignature(compute_pos_root_signature_.Get());

    commandList->SetComputeRootUnorderedAccessView(ComputePositionsRootSignatureParams::ParticlePositionsBufferSlot, particle_pos_buffer_->GetGPUVirtualAddress());
    commandList->SetComputeRootConstantBufferView(ComputePositionsRootSignatureParams::ConstantBufferSlot, application_->GetComputeCB()->GetGPUVirtualAddress());

    commandList->Dispatch(particle_pos_manip_threadgroups_, 1, 1);

    commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(particle_pos_buffer_.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ));
}

void Computer::ComputeSDFTexture()
{
    auto commandList = device_resources_->GetCommandList();

    commandList->SetPipelineState(compute_tex_state_object_.Get());
    commandList->SetComputeRootSignature(compute_tex_root_signature_.Get());

    ID3D12DescriptorHeap* heap = application_->GetDescriptorHeap();
    commandList->SetDescriptorHeaps(1, &heap);

    commandList->SetComputeRootShaderResourceView(ComputeTextureRootSignatureParams::ParticlePositionsBufferSlot, particle_pos_buffer_->GetGPUVirtualAddress());
    commandList->SetComputeRootDescriptorTable(ComputeTextureRootSignatureParams::TextureSlot, sdf_3d_texture_gpu_handle_);

    commandList->Dispatch(tex_creation_threadgroups_.x, tex_creation_threadgroups_.y, tex_creation_threadgroups_.z);

    commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(sdf_3d_texture_.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ));
}

void Computer::CreateRootSignatures()
{
    // Root signature used for shader which computes particle positions
    CD3DX12_ROOT_PARAMETER pos_root_params[ComputePositionsRootSignatureParams::Count];
    pos_root_params[ComputePositionsRootSignatureParams::ParticlePositionsBufferSlot].InitAsUnorderedAccessView(0);
    pos_root_params[ComputePositionsRootSignatureParams::ConstantBufferSlot].InitAsConstantBufferView(0);
    CD3DX12_ROOT_SIGNATURE_DESC pos_root_signature_desc(ARRAYSIZE(pos_root_params), pos_root_params);
    SerializeAndCreateComputeRootSignature(pos_root_signature_desc, &compute_pos_root_signature_);
    
    // Root signature used for shader which computes SDF texture
    CD3DX12_DESCRIPTOR_RANGE uav_descriptor;
    uav_descriptor.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
    CD3DX12_ROOT_PARAMETER tex_root_params[ComputeTextureRootSignatureParams::Count];
    tex_root_params[ComputeTextureRootSignatureParams::ParticlePositionsBufferSlot].InitAsShaderResourceView(0);
    tex_root_params[ComputeTextureRootSignatureParams::TextureSlot].InitAsDescriptorTable(1, &uav_descriptor);
    CD3DX12_ROOT_SIGNATURE_DESC tex_root_signature_desc(ARRAYSIZE(tex_root_params), tex_root_params);
    SerializeAndCreateComputeRootSignature(tex_root_signature_desc, &compute_tex_root_signature_);
}

void Computer::SerializeAndCreateComputeRootSignature(D3D12_ROOT_SIGNATURE_DESC& desc, ComPtr<ID3D12RootSignature>* rootSig)
{
    ComPtr<ID3DBlob> blob;
    ComPtr<ID3DBlob> error;

    ThrowIfFailed(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error), error ? static_cast<wchar_t*>(error->GetBufferPointer()) : nullptr);
    ThrowIfFailed(device_resources_->GetD3DDevice()->CreateRootSignature(1, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&(*rootSig))));
}

void Computer::CreateComputePipelineStateObjects()
{
    ComPtr<ID3DBlob> compute_shader;
    ComPtr<ID3DBlob> error_blob;
#if defined(_DEBUG)
    // Enable better shader debugging with the graphics debugging tools.
    UINT flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    UINT flags = 0;
#endif

    // Compute positions shader
    if (FAILED(D3DCompileFromFile(application_->GetAssetFullPath(L"ComputePositions.hlsl").c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "CSPosMain", "cs_5_1", flags, 0, &compute_shader, &error_blob))) {
        std::string errMsg((char*)error_blob->GetBufferPointer(), error_blob->GetBufferSize());        
        throw std::exception(errMsg.c_str());
    }

    D3D12_COMPUTE_PIPELINE_STATE_DESC compute_pso = {};
    compute_pso.pRootSignature = compute_pos_root_signature_.Get();
    compute_pso.CS = CD3DX12_SHADER_BYTECODE(compute_shader.Get());
    compute_pso.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
    ThrowIfFailed(device_resources_->GetD3DDevice()->CreateComputePipelineState(&compute_pso, IID_PPV_ARGS(&compute_pos_state_object_)));

    // Compute texture shader
    if (FAILED(D3DCompileFromFile(application_->GetAssetFullPath(L"ComputeTexture.hlsl").c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "CSTexMain", "cs_5_1", flags, 0, &compute_shader, &error_blob))) {
        std::string errMsg((char*)error_blob->GetBufferPointer(), error_blob->GetBufferSize());
        throw std::exception(errMsg.c_str());
    }
    
    compute_pso.pRootSignature = compute_tex_root_signature_.Get();
    compute_pso.CS = CD3DX12_SHADER_BYTECODE(compute_shader.Get());
    ThrowIfFailed(device_resources_->GetD3DDevice()->CreateComputePipelineState(&compute_pso, IID_PPV_ARGS(&compute_tex_state_object_)));
}

void Computer::CreateBuffers()
{
    auto command_list = device_resources_->GetCommandList();
    auto device = device_resources_->GetD3DDevice();
    UINT descriptor_size = device_resources_->GetD3DDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // Calculate threadgroup sizes
    particle_pos_manip_threadgroups_ = std::ceil(NUM_PARTICLES / 1024.f);
    tex_creation_threadgroups_ = XMUINT3(std::ceil(TEXTURE_RESOLUTION / 32.f), std::ceil(TEXTURE_RESOLUTION / 32.f), TEXTURE_RESOLUTION);

    ParticlePosition positions[NUM_PARTICLES];

    for (int i = 0; i < NUM_PARTICLES; i++) {
        positions[i].position_.x = ((float)(rand() % 10) + 0.5f) / 10.f;
        positions[i].position_.y = ((float)(rand() % 10) + 0.5f) / 10.f;
        positions[i].position_.z = ((float)(rand() % 10) + 0.5f) / 10.f;
        positions[i].speed_ = rand() % 10 + 1;
        positions[i].start_y_ = positions[i].position_.y;
    }

    UINT64 byte_size = NUM_PARTICLES * sizeof(ParticlePosition);

    particle_pos_buffer_ = Utilities::CreateDefaultBuffer(device, command_list, &positions, byte_size, particle_pos_buffer_uploader_, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
}

void Computer::CreateTexture3D()
{
    // Create the output resource. The dimensions and format should match the swap-chain.
    auto uavDesc = CD3DX12_RESOURCE_DESC::Tex3D(DXGI_FORMAT_R8_SNORM, TEXTURE_RESOLUTION, TEXTURE_RESOLUTION, TEXTURE_RESOLUTION, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    ThrowIfFailed(device_resources_->GetD3DDevice()->CreateCommittedResource(
        &defaultHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &uavDesc,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        nullptr, IID_PPV_ARGS(&sdf_3d_texture_)));

    D3D12_CPU_DESCRIPTOR_HANDLE uav_handle;
    UINT heap_index = application_->AllocateDescriptor(&uav_handle);

    /*D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
    uav_desc.Format = DXGI_FORMAT_R8_SNORM;
    uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;*/
    device_resources_->GetD3DDevice()->CreateUnorderedAccessView(sdf_3d_texture_.Get(), nullptr, nullptr, uav_handle);

    UINT descriptor_size = device_resources_->GetD3DDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    sdf_3d_texture_gpu_handle_ = CD3DX12_GPU_DESCRIPTOR_HANDLE(application_->GetDescriptorHeap()->GetGPUDescriptorHandleForHeapStart(), heap_index, descriptor_size);
}
