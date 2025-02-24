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

    commandList->Dispatch(particle_threadgroups_, 1, 1);

    commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(particle_pos_buffer_.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ));
}

void Computer::ComputeGrid()
{
    auto commandList = device_resources_->GetCommandList();

    // Bind resources
    commandList->SetComputeRootSignature(compute_grid_root_signature_.Get());
    commandList->SetComputeRootUnorderedAccessView(ComputeGridRootSignatureParams::ParticlePositionsBufferSlot, particle_pos_buffer_->GetGPUVirtualAddress());
    commandList->SetComputeRootUnorderedAccessView(ComputeGridRootSignatureParams::CellsSlot, cells_buffer_->GetGPUVirtualAddress());
    commandList->SetComputeRootUnorderedAccessView(ComputeGridRootSignatureParams::BlocksSlot, blocks_buffer_->GetGPUVirtualAddress());
    commandList->SetComputeRootUnorderedAccessView(ComputeGridRootSignatureParams::SurfaceBlocksSlot, surface_block_indices_buffer_->GetGPUVirtualAddress());
    commandList->SetComputeRootUnorderedAccessView(ComputeGridRootSignatureParams::SurfaceCellsSlot, surface_cell_indices_buffer_->GetGPUVirtualAddress());
    commandList->SetComputeRootUnorderedAccessView(ComputeGridRootSignatureParams::SurfaceCountsSlot, surface_counts_buffer_->GetGPUVirtualAddress());

    // Clear counters
    commandList->SetPipelineState(compute_clear_counts_state_object_.Get());
    commandList->Dispatch(1, 1, 1);

    // Build the grid structure
    commandList->SetPipelineState(compute_grid_state_object_.Get());
    commandList->Dispatch(particle_threadgroups_, 1, 1);

    // Detect surface blocks
    commandList->SetPipelineState(compute_surface_blocks_state_object_.Get());
    commandList->Dispatch(blocks_threadgroups_, 1, 1);

    commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(surface_counts_buffer_.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ));

    // Set dispatch threadgroups for surface cell detection
    commandList->SetPipelineState(compute_dispatch_surface_cells_state_object_.Get());
    commandList->SetComputeRootSignature(compute_dispatch_surface_cells_root_signature_.Get());
    commandList->SetComputeRootShaderResourceView(ComputeDispatchCellsRootSignatureParams::SurfaceCountsSlot, surface_counts_buffer_->GetGPUVirtualAddress());
    commandList->SetComputeRootUnorderedAccessView(ComputeDispatchCellsRootSignatureParams::DispatchArgsSlot, surface_cells_dispatch_buffer_->GetGPUVirtualAddress());

    commandList->Dispatch(1, 1, 1);

    commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(surface_counts_buffer_.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

    // Detect surface cells
    commandList->SetPipelineState(compute_surface_cells_state_object_.Get());
    commandList->SetComputeRootSignature(compute_grid_root_signature_.Get());

    commandList->SetComputeRootUnorderedAccessView(ComputeGridRootSignatureParams::ParticlePositionsBufferSlot, particle_pos_buffer_->GetGPUVirtualAddress());
    commandList->SetComputeRootUnorderedAccessView(ComputeGridRootSignatureParams::CellsSlot, cells_buffer_->GetGPUVirtualAddress());
    commandList->SetComputeRootUnorderedAccessView(ComputeGridRootSignatureParams::BlocksSlot, blocks_buffer_->GetGPUVirtualAddress());
    commandList->SetComputeRootUnorderedAccessView(ComputeGridRootSignatureParams::SurfaceBlocksSlot, surface_block_indices_buffer_->GetGPUVirtualAddress());
    commandList->SetComputeRootUnorderedAccessView(ComputeGridRootSignatureParams::SurfaceCellsSlot, surface_cell_indices_buffer_->GetGPUVirtualAddress());
    commandList->SetComputeRootUnorderedAccessView(ComputeGridRootSignatureParams::SurfaceCountsSlot, surface_counts_buffer_->GetGPUVirtualAddress());
    commandList->ExecuteIndirect(compute_surface_cells_command_signature_.Get(), 1, surface_cells_dispatch_buffer_.Get(), 0, nullptr, 0);
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
    
    // Root signature used for shader which computes the grid
    CD3DX12_ROOT_PARAMETER grid_root_params[ComputeGridRootSignatureParams::Count];
    grid_root_params[ComputeGridRootSignatureParams::ParticlePositionsBufferSlot].InitAsUnorderedAccessView(0);
    grid_root_params[ComputeGridRootSignatureParams::CellsSlot].InitAsUnorderedAccessView(1);
    grid_root_params[ComputeGridRootSignatureParams::BlocksSlot].InitAsUnorderedAccessView(2);
    grid_root_params[ComputeGridRootSignatureParams::SurfaceBlocksSlot].InitAsUnorderedAccessView(3);
    grid_root_params[ComputeGridRootSignatureParams::SurfaceCellsSlot].InitAsUnorderedAccessView(4);
    grid_root_params[ComputeGridRootSignatureParams::SurfaceCountsSlot].InitAsUnorderedAccessView(5);
    CD3DX12_ROOT_SIGNATURE_DESC grid_root_signature_desc(ARRAYSIZE(grid_root_params), grid_root_params);
    SerializeAndCreateComputeRootSignature(grid_root_signature_desc, &compute_grid_root_signature_);

    // Root signature used for setting number of threadgroups to dispatch for surface cell detection
    CD3DX12_ROOT_PARAMETER cell_dispatch_root_params[ComputeDispatchCellsRootSignatureParams::Count];
    cell_dispatch_root_params[ComputeDispatchCellsRootSignatureParams::SurfaceCountsSlot].InitAsShaderResourceView(0);
    cell_dispatch_root_params[ComputeDispatchCellsRootSignatureParams::DispatchArgsSlot].InitAsUnorderedAccessView(0);
    CD3DX12_ROOT_SIGNATURE_DESC cell_dispatch_signature_desc(ARRAYSIZE(cell_dispatch_root_params), cell_dispatch_root_params);
    SerializeAndCreateComputeRootSignature(cell_dispatch_signature_desc, &compute_dispatch_surface_cells_root_signature_);

    // Root signature used for shader which computes SDF texture
    CD3DX12_DESCRIPTOR_RANGE uav_descriptor;
    uav_descriptor.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
    CD3DX12_ROOT_PARAMETER tex_root_params[ComputeTextureRootSignatureParams::Count];
    tex_root_params[ComputeTextureRootSignatureParams::ParticlePositionsBufferSlot].InitAsShaderResourceView(0);
    tex_root_params[ComputeTextureRootSignatureParams::TextureSlot].InitAsDescriptorTable(1, &uav_descriptor);
    CD3DX12_ROOT_SIGNATURE_DESC tex_root_signature_desc(ARRAYSIZE(tex_root_params), tex_root_params);
    SerializeAndCreateComputeRootSignature(tex_root_signature_desc, &compute_tex_root_signature_);

    // Create dispatch command signature for indirect dispatch of surface cell detection
    D3D12_COMMAND_SIGNATURE_DESC command_signature_desc = {};
    command_signature_desc.ByteStride = sizeof(D3D12_DISPATCH_ARGUMENTS);
    command_signature_desc.NumArgumentDescs = 1;
    D3D12_INDIRECT_ARGUMENT_DESC argument_desc = {};
    argument_desc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;
    command_signature_desc.pArgumentDescs = &argument_desc;
    device_resources_->GetD3DDevice()->CreateCommandSignature(&command_signature_desc, nullptr, IID_PPV_ARGS(&compute_surface_cells_command_signature_));

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

    // Build grid shader
    if (FAILED(D3DCompileFromFile(application_->GetAssetFullPath(L"ComputeGrid.hlsl").c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "CSGridMain", "cs_5_1", flags, 0, &compute_shader, &error_blob))) {
        std::string errMsg((char*)error_blob->GetBufferPointer(), error_blob->GetBufferSize());
        throw std::exception(errMsg.c_str());
    }
    
    compute_pso.pRootSignature = compute_grid_root_signature_.Get();
    compute_pso.CS = CD3DX12_SHADER_BYTECODE(compute_shader.Get());
    ThrowIfFailed(device_resources_->GetD3DDevice()->CreateComputePipelineState(&compute_pso, IID_PPV_ARGS(&compute_grid_state_object_)));

    // Shader to clear counters
    if (FAILED(D3DCompileFromFile(application_->GetAssetFullPath(L"ComputeGrid.hlsl").c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "CSClearGridCounts", "cs_5_1", flags, 0, &compute_shader, &error_blob))) {
        std::string errMsg((char*)error_blob->GetBufferPointer(), error_blob->GetBufferSize());
        throw std::exception(errMsg.c_str());
    }
    
    compute_pso.CS = CD3DX12_SHADER_BYTECODE(compute_shader.Get());
    ThrowIfFailed(device_resources_->GetD3DDevice()->CreateComputePipelineState(&compute_pso, IID_PPV_ARGS(&compute_clear_counts_state_object_)));

    // Detect surface blocks shader
    if (FAILED(D3DCompileFromFile(application_->GetAssetFullPath(L"ComputeGrid.hlsl").c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "CSDetectSurfaceBlocksMain", "cs_5_1", flags, 0, &compute_shader, &error_blob))) {
        std::string errMsg((char*)error_blob->GetBufferPointer(), error_blob->GetBufferSize());
        throw std::exception(errMsg.c_str());
    }
    
    //compute_pso.pRootSignature = compute_surface_blocks_root_signature_.Get();
    compute_pso.CS = CD3DX12_SHADER_BYTECODE(compute_shader.Get());
    ThrowIfFailed(device_resources_->GetD3DDevice()->CreateComputePipelineState(&compute_pso, IID_PPV_ARGS(&compute_surface_blocks_state_object_)));

    // Detect surface cells shader
    if (FAILED(D3DCompileFromFile(application_->GetAssetFullPath(L"ComputeGrid.hlsl").c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "CSDetectSurfaceCellsMain", "cs_5_1", flags, 0, &compute_shader, &error_blob))) {
        std::string errMsg((char*)error_blob->GetBufferPointer(), error_blob->GetBufferSize());
        throw std::exception(errMsg.c_str());
    }
    
    //compute_pso.pRootSignature = compute_surface_cells_root_signature_.Get();
    compute_pso.CS = CD3DX12_SHADER_BYTECODE(compute_shader.Get());
    ThrowIfFailed(device_resources_->GetD3DDevice()->CreateComputePipelineState(&compute_pso, IID_PPV_ARGS(&compute_surface_cells_state_object_)));


    // Compute texture shader
    if (FAILED(D3DCompileFromFile(application_->GetAssetFullPath(L"ComputeDispatchSurfaceCells.hlsl").c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "CSDispatchSurfaceCellDetection", "cs_5_1", flags, 0, &compute_shader, &error_blob))) {
        std::string errMsg((char*)error_blob->GetBufferPointer(), error_blob->GetBufferSize());
        throw std::exception(errMsg.c_str());
    }
    
    compute_pso.pRootSignature = compute_dispatch_surface_cells_root_signature_.Get();
    compute_pso.CS = CD3DX12_SHADER_BYTECODE(compute_shader.Get());
    ThrowIfFailed(device_resources_->GetD3DDevice()->CreateComputePipelineState(&compute_pso, IID_PPV_ARGS(&compute_dispatch_surface_cells_state_object_)));

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
    particle_threadgroups_ = std::ceil(NUM_PARTICLES / 1024.f);
    blocks_threadgroups_ = std::ceil(NUM_BLOCKS / 1024.f);
    tex_creation_threadgroups_ = XMUINT3(std::ceil(TEXTURE_RESOLUTION / 32.f), std::ceil(TEXTURE_RESOLUTION / 32.f), TEXTURE_RESOLUTION);

    // Particle position buffer
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

    // Grid buffers
    Utilities::AllocateDefaultBuffer(device, NUM_CELLS * sizeof(Cell), cells_buffer_.GetAddressOf(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    Utilities::AllocateDefaultBuffer(device, NUM_BLOCKS * sizeof(Block), blocks_buffer_.GetAddressOf(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    Utilities::AllocateDefaultBuffer(device, NUM_CELLS * sizeof(unsigned int), surface_cell_indices_buffer_.GetAddressOf(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    Utilities::AllocateDefaultBuffer(device, NUM_BLOCKS * sizeof(unsigned int), surface_block_indices_buffer_.GetAddressOf(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    Utilities::AllocateDefaultBuffer(device, sizeof(GridSurfaceCounts), surface_counts_buffer_.GetAddressOf(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);


    // Indirect dispatch argument buffer for surface cell detection
    D3D12_DISPATCH_ARGUMENTS dispatch_args = { 1, 1, 1 }; // By default dispatch (1, 1, 1) thread groups
    surface_cells_dispatch_buffer_ = Utilities::CreateDefaultBuffer(device, command_list, &dispatch_args, sizeof(D3D12_DISPATCH_ARGUMENTS), surface_cells_dispatch_buffer_uploader_, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);



    //// AABB buffer
    //AABB aabbs[8];
    //aabbs[0].max_ = XMFLOAT3(1.f, 1.f, 1.f);
    //aabbs[0].min_ = XMFLOAT3(0.f, 0.f, 0.f);

    //aabbs[1].max_ = XMFLOAT3(2.f, 1.f, 1.f);
    //aabbs[1].min_ = XMFLOAT3(1.f, 0.f, 0.f);

    //aabbs[2].max_ = XMFLOAT3(1.f, 1.f, 2.f);
    //aabbs[2].min_ = XMFLOAT3(0.f, 0.f, 1.f);

    //aabbs[3].max_ = XMFLOAT3(2.f, 1.f, 2.f);
    //aabbs[3].min_ = XMFLOAT3(1.f, 1.f, 1.f);

    //aabbs[4].max_ = XMFLOAT3(1.f, 2.f, 1.f);
    //aabbs[4].min_ = XMFLOAT3(0.f, 1.f, 0.f);

    //aabbs[5].max_ = XMFLOAT3(2.f, 2.f, 1.f);
    //aabbs[5].min_ = XMFLOAT3(1.f, 1.f, 0.f);
   
    //aabbs[6].max_ = XMFLOAT3(1.f, 2.f, 2.f);
    //aabbs[6].min_ = XMFLOAT3(0.f, 1.f, 1.f);

    //aabbs[7].max_ = XMFLOAT3(2.f, 2.f, 2.f);
    //aabbs[7].min_ = XMFLOAT3(1.f, 1.f, 1.f);

    //aabb_buffer_ = Utilities::CreateDefaultBuffer(device, command_list, &aabbs, sizeof(aabbs), aabb_buffer_uploader_, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
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
