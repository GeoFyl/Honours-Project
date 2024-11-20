#include "stdafx.h"
#include "Computer.h"
#include "HonoursApplication.h"

Computer::Computer(DX::DeviceResources* device_resources, HonoursApplication* app) :
	device_resources_(device_resources), application_(app)
{
    device_resources_->GetCommandList()->Reset(device_resources_->GetCommandAllocator(), nullptr);

	CreateRootSignatures();
	CreateComputePipelineStateObject();

    // Execute command list and wait until assets have been uploaded to the GPU.
    device_resources_->ExecuteCommandList();
    device_resources_->WaitForGpu();

    ReleaseUploaders();
}

void Computer::Compute()
{
    auto commandList = device_resources_->GetCommandList();

    commandList->SetPipelineState(compute_state_object_.Get());
    commandList->SetComputeRootSignature(compute_global_root_signature_.Get());

    // Bind the heaps, acceleration structure and dispatch rays.
    ID3D12DescriptorHeap* heap = application_->GetDescriptorHeap();
    commandList->SetDescriptorHeaps(1, &heap);

    consume_buffer_index_ = 1 - consume_buffer_index_;
    if (consume_buffer_index_ == 0) {
        commandList->SetComputeRootDescriptorTable(GlobalComputeRootSignatureParams::ParticlePositionsConsumeBufferSlot, particle_pos_buffer_0_gpu_handle_);
        commandList->SetComputeRootDescriptorTable(GlobalComputeRootSignatureParams::ParticlePositionsAppendBufferSlot, particle_pos_buffer_1_gpu_handle_);
    }
    else {
        commandList->SetComputeRootDescriptorTable(GlobalComputeRootSignatureParams::ParticlePositionsConsumeBufferSlot, particle_pos_buffer_1_gpu_handle_);
        commandList->SetComputeRootDescriptorTable(GlobalComputeRootSignatureParams::ParticlePositionsAppendBufferSlot, particle_pos_buffer_0_gpu_handle_);
    }

    commandList->Dispatch(1, 1, 1);
}

void Computer::CreateRootSignatures()
{
    // Global Root Signature
    // This is a root signature that is shared across all compute shaders invoked during a Dispatch() call.
    {
        CD3DX12_DESCRIPTOR_RANGE ParticleConsumeDescriptor;
        CD3DX12_DESCRIPTOR_RANGE ParticleAppendDescriptor;
        ParticleConsumeDescriptor.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
        ParticleAppendDescriptor.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1);

        CD3DX12_ROOT_PARAMETER rootParameters[GlobalComputeRootSignatureParams::Count];
        rootParameters[GlobalComputeRootSignatureParams::ParticlePositionsConsumeBufferSlot].InitAsDescriptorTable(1, &ParticleConsumeDescriptor);
        rootParameters[GlobalComputeRootSignatureParams::ParticlePositionsAppendBufferSlot].InitAsDescriptorTable(1, &ParticleAppendDescriptor);
        CD3DX12_ROOT_SIGNATURE_DESC globalRootSignatureDesc(ARRAYSIZE(rootParameters), rootParameters);
        SerializeAndCreateComputeRootSignature(globalRootSignatureDesc, &compute_global_root_signature_);
    }
}

void Computer::SerializeAndCreateComputeRootSignature(D3D12_ROOT_SIGNATURE_DESC& desc, ComPtr<ID3D12RootSignature>* rootSig)
{
    ComPtr<ID3DBlob> blob;
    ComPtr<ID3DBlob> error;

    ThrowIfFailed(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error), error ? static_cast<wchar_t*>(error->GetBufferPointer()) : nullptr);
    ThrowIfFailed(device_resources_->GetD3DDevice()->CreateRootSignature(1, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&(*rootSig))));
}

void Computer::CreateComputePipelineStateObject()
{
    ComPtr<ID3DBlob> compute_shader;
    ComPtr<ID3DBlob> error_blob;
#if defined(_DEBUG)
    // Enable better shader debugging with the graphics debugging tools.
    UINT flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    UINT flags = 0;
#endif

   // ThrowIfFailed(D3DCompileFromFile(application_->GetAssetFullPath(L"Compute.hlsl").c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "CSMain", "cs_5_0", flags, 0, &compute_shader, &error_blob));
    if (FAILED(D3DCompileFromFile(application_->GetAssetFullPath(L"Compute.hlsl").c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "CSMain", "cs_5_0", flags, 0, &compute_shader, &error_blob))) {
        std::string errMsg((char*)error_blob->GetBufferPointer(), error_blob->GetBufferSize());        
        throw std::exception(errMsg.c_str());
    }

    D3D12_COMPUTE_PIPELINE_STATE_DESC compute_pso = {};
    compute_pso.pRootSignature = compute_global_root_signature_.Get();
    compute_pso.CS = CD3DX12_SHADER_BYTECODE(compute_shader.Get());
    compute_pso.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
    ThrowIfFailed(device_resources_->GetD3DDevice()->CreateComputePipelineState(&compute_pso, IID_PPV_ARGS(&compute_state_object_)));
}

void Computer::CreateBuffers()
{
    auto command_list = device_resources_->GetCommandList();
    auto device = device_resources_->GetD3DDevice();
    UINT descriptor_size = device_resources_->GetD3DDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    ParticlePosition positions[NUM_PARTICLES];
    for (int i = 0; i < NUM_PARTICLES; i++) {
        positions[i].position_.x = ((float)(rand() % 10) + 0.5f) / 10.f;
        positions[i].position_.y = ((float)(rand() % 10) + 0.5f) / 10.f;
        positions[i].position_.z = ((float)(rand() % 10) + 0.5f) / 10.f;
        positions[i].speed = rand() % 10 + 1;
    }

    UINT64 byte_size = NUM_PARTICLES * sizeof(ParticlePosition);

    // Describe data being copied
    D3D12_SUBRESOURCE_DATA data_desc = {};
    data_desc.pData = &positions;
    data_desc.RowPitch = byte_size;
    data_desc.SlicePitch = data_desc.RowPitch;

    // Create intermediate upload resource
    ThrowIfFailed(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(byte_size),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(particle_pos_buffer_uploader_.GetAddressOf())));

    Utilities::AllocateDefaultBuffer(device, byte_size, particle_pos_buffer_0_.GetAddressOf(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    Utilities::AllocateDefaultBuffer(device, byte_size, particle_pos_buffer_1_.GetAddressOf(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(particle_pos_buffer_0_.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST));

    // Schedule copy - must keep upload buffer alive till copy has been executed!
    UpdateSubresources<1>(command_list, particle_pos_buffer_0_.Get(), particle_pos_buffer_uploader_.Get(), 0, 0, 1, &data_desc);

    command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(particle_pos_buffer_0_.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

    // Create descriptors and views
    D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
    UAVDesc.Format = DXGI_FORMAT_UNKNOWN;

    // buffer 0
    D3D12_CPU_DESCRIPTOR_HANDLE uav_handle_0;
    UINT heap_index_0 = application_->AllocateDescriptor(&uav_handle_0);

    device->CreateUnorderedAccessView(particle_pos_buffer_0_.Get(), particle_pos_buffer_0_counter_.Get(), &UAVDesc, uav_handle_0);

    particle_pos_buffer_0_gpu_handle_ = CD3DX12_GPU_DESCRIPTOR_HANDLE(application_->GetDescriptorHeap()->GetGPUDescriptorHandleForHeapStart(), heap_index_0, descriptor_size);

    // buffer 1
    D3D12_CPU_DESCRIPTOR_HANDLE uav_handle_1;
    UINT heap_index_1 = application_->AllocateDescriptor(&uav_handle_1);

    device->CreateUnorderedAccessView(particle_pos_buffer_1_.Get(), particle_pos_buffer_1_counter_.Get(), &UAVDesc, uav_handle_1);

    particle_pos_buffer_1_gpu_handle_ = CD3DX12_GPU_DESCRIPTOR_HANDLE(application_->GetDescriptorHeap()->GetGPUDescriptorHandleForHeapStart(), heap_index_1, descriptor_size);


}
