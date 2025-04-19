#include "stdafx.h"
#include "Computer.h"
#include "HonoursApplication.h"
#include "RayTracer.h"


Computer::Computer(DX::DeviceResources* device_resources, HonoursApplication* app) :
	device_resources_(device_resources), application_(app)
{
    device_resources_->GetCommandList()->Reset(device_resources_->GetCommandAllocator(), nullptr);

	CreateRootSignatures();
	CreateComputePipelineStateObjects();
    CreateBuffers();
    AllocateSimpleSDFTexture();

    // Execute command list and wait until assets have been uploaded to the GPU.
    device_resources_->ExecuteCommandList();
    device_resources_->WaitForGpu();
    device_resources_->ResetCommandList();

    compute_cb_ = std::make_unique<UploadBuffer<ComputeCB>>(device_resources_->GetD3DDevice(), 1, true);
    test_vals_cb_ = std::make_unique<UploadBuffer<TestVariables>>(device_resources_->GetD3DDevice(), 1, true);

    // Upload defined test values
    test_vals_cb_->Values() = test_vars_;
    test_vals_cb_->CopyData(0);

    GenerateParticles(); // Generate initial particle positions

    // Execute command list and wait until assets have been uploaded to the GPU.
    device_resources_->ExecuteCommandList();
    device_resources_->WaitForGpu();

    ReleaseUploaders();
}

void Computer::GenerateParticles()
{
    auto command_list = device_resources_->GetCommandList();

    command_list->SetPipelineState(compute_generate_state_object_.Get());
    command_list->SetComputeRootSignature(compute_pos_root_signature_.Get());

    command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(particle_buffer_unordered_.Get()));

    command_list->SetComputeRootUnorderedAccessView(ComputePositionsRootSignatureParams::ParticlePositionsBufferSlot, particle_buffer_unordered_->GetGPUVirtualAddress());
    //command_list->SetComputeRootConstantBufferView(ComputePositionsRootSignatureParams::ConstantBufferSlot, compute_cb_->Resource()->GetGPUVirtualAddress());
    command_list->SetComputeRootConstantBufferView(ComputePositionsRootSignatureParams::TestValuesSlot, test_vals_cb_->Resource()->GetGPUVirtualAddress());

    command_list->Dispatch(particle_threadgroups_, 1, 1);
}

void Computer::ComputePostitions()
{
   
    auto command_list = device_resources_->GetCommandList();

    command_list->SetPipelineState(compute_pos_state_object_.Get());
    command_list->SetComputeRootSignature(compute_pos_root_signature_.Get());

    //command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(particle_pos_buffer_.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

    command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(particle_buffer_unordered_.Get()));

    command_list->SetComputeRootUnorderedAccessView(ComputePositionsRootSignatureParams::ParticlePositionsBufferSlot, particle_buffer_unordered_->GetGPUVirtualAddress());
    command_list->SetComputeRootConstantBufferView(ComputePositionsRootSignatureParams::ConstantBufferSlot, compute_cb_->Resource()->GetGPUVirtualAddress());
    command_list->SetComputeRootConstantBufferView(ComputePositionsRootSignatureParams::TestValuesSlot, test_vals_cb_->Resource()->GetGPUVirtualAddress());

    command_list->Dispatch(particle_threadgroups_, 1, 1);

    //command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(particle_pos_buffer_.Get()));

    //command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(particle_pos_buffer_.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ));

}

void Computer::ComputeGrid()
{
    auto command_list = device_resources_->GetCommandList();


    // Bind resources
    command_list->SetComputeRootSignature(compute_grid_root_signature_.Get());

    command_list->SetComputeRootUnorderedAccessView(ComputeGridRootSignatureParams::ParticlePositionsBufferSlot, particle_buffer_unordered_->GetGPUVirtualAddress());
    command_list->SetComputeRootUnorderedAccessView(ComputeGridRootSignatureParams::CellsSlot, scan_shader_->GetScanInBuffer()->GetGPUVirtualAddress());
    command_list->SetComputeRootUnorderedAccessView(ComputeGridRootSignatureParams::BlocksSlot, blocks_buffer_->GetGPUVirtualAddress());
    command_list->SetComputeRootUnorderedAccessView(ComputeGridRootSignatureParams::SurfaceBlocksSlot, surface_block_indices_buffer_->GetGPUVirtualAddress());
    command_list->SetComputeRootUnorderedAccessView(ComputeGridRootSignatureParams::SurfaceCellsSlot, surface_cell_indices_buffer_->GetGPUVirtualAddress());
    command_list->SetComputeRootUnorderedAccessView(ComputeGridRootSignatureParams::SurfaceCountsSlot, surface_counts_buffer_->GetGPUVirtualAddress());
    command_list->SetComputeRootConstantBufferView(ComputeGridRootSignatureParams::TestValuesSlot, test_vals_cb_->Resource()->GetGPUVirtualAddress());

    D3D12_RESOURCE_BARRIER grid_uav_barriers[6];
    grid_uav_barriers[0] = CD3DX12_RESOURCE_BARRIER::UAV(scan_shader_->GetScanInBuffer());
    grid_uav_barriers[1] = CD3DX12_RESOURCE_BARRIER::UAV(blocks_buffer_.Get());
    grid_uav_barriers[2] = CD3DX12_RESOURCE_BARRIER::UAV(surface_block_indices_buffer_.Get());
    grid_uav_barriers[3] = CD3DX12_RESOURCE_BARRIER::UAV(surface_cell_indices_buffer_.Get());
    grid_uav_barriers[4] = CD3DX12_RESOURCE_BARRIER::UAV(surface_counts_buffer_.Get());
    grid_uav_barriers[5] = CD3DX12_RESOURCE_BARRIER::UAV(particle_buffer_unordered_.Get());

    command_list->ResourceBarrier(ARRAYSIZE(grid_uav_barriers), grid_uav_barriers);

    // Clear counters
    command_list->SetPipelineState(compute_clear_counts_state_object_.Get());
    command_list->Dispatch(clear_counts_threadgroups_, 1, 1);

    command_list->ResourceBarrier(ARRAYSIZE(grid_uav_barriers), grid_uav_barriers);

    // Build the grid structure
    command_list->SetPipelineState(compute_grid_state_object_.Get());
    command_list->Dispatch(particle_threadgroups_, 1, 1);

    command_list->ResourceBarrier(ARRAYSIZE(grid_uav_barriers), grid_uav_barriers);

    // Detect surface blocks
    command_list->SetPipelineState(compute_surface_blocks_state_object_.Get());
    command_list->Dispatch(blocks_threadgroups_, 1, 1);
    
    command_list->ResourceBarrier(ARRAYSIZE(grid_uav_barriers), grid_uav_barriers);

    //command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(surface_counts_buffer_.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ));

    ReadBackBlocksCount();

    // Set dispatch threadgroups for surface cell detection
   /* command_list->SetPipelineState(compute_dispatch_surface_cells_state_object_.Get());
    command_list->SetComputeRootSignature(compute_dispatch_surface_cells_root_signature_.Get());
    command_list->SetComputeRootShaderResourceView(ComputeDispatchCellsRootSignatureParams::SurfaceCountsSlot, surface_counts_buffer_->GetGPUVirtualAddress());
    command_list->SetComputeRootUnorderedAccessView(ComputeDispatchCellsRootSignatureParams::DispatchArgsSlot, surface_cells_dispatch_buffer_->GetGPUVirtualAddress());

    command_list->Dispatch(1, 1, 1);

    command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(surface_cells_dispatch_buffer_.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT));
    command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(surface_counts_buffer_.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));*/


    if (surface_blocks_count_ > 0) {
        // Detect surface cells
        command_list->SetPipelineState(compute_surface_cells_state_object_.Get());
        command_list->SetComputeRootSignature(compute_grid_root_signature_.Get());

        command_list->SetComputeRootUnorderedAccessView(ComputeGridRootSignatureParams::ParticlePositionsBufferSlot, particle_buffer_unordered_->GetGPUVirtualAddress());
        command_list->SetComputeRootUnorderedAccessView(ComputeGridRootSignatureParams::CellsSlot, scan_shader_->GetScanInBuffer()->GetGPUVirtualAddress());
        command_list->SetComputeRootUnorderedAccessView(ComputeGridRootSignatureParams::BlocksSlot, blocks_buffer_->GetGPUVirtualAddress());
        command_list->SetComputeRootUnorderedAccessView(ComputeGridRootSignatureParams::SurfaceBlocksSlot, surface_block_indices_buffer_->GetGPUVirtualAddress());
        command_list->SetComputeRootUnorderedAccessView(ComputeGridRootSignatureParams::SurfaceCellsSlot, surface_cell_indices_buffer_->GetGPUVirtualAddress());
        command_list->SetComputeRootUnorderedAccessView(ComputeGridRootSignatureParams::SurfaceCountsSlot, surface_counts_buffer_->GetGPUVirtualAddress());
        command_list->SetComputeRootConstantBufferView(ComputeGridRootSignatureParams::TestValuesSlot, test_vals_cb_->Resource()->GetGPUVirtualAddress());

        command_list->ResourceBarrier(ARRAYSIZE(grid_uav_barriers), grid_uav_barriers);

        command_list->Dispatch(surface_blocks_count_, 1, 1);
    }
    
    command_list->ResourceBarrier(ARRAYSIZE(grid_uav_barriers), grid_uav_barriers);


   // command_list->ExecuteIndirect(compute_surface_cells_command_signature_.Get(), 1, surface_cells_dispatch_buffer_.Get(), 0, nullptr, 0);

   // command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(surface_cells_dispatch_buffer_.Get(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
}

void Computer::ComputeAABBs()
{
    auto command_list = device_resources_->GetCommandList();

    // Read back new count of surface cells
    ReadBackCellCount();

    if (bricks_count_ > 0) {
        device_resources_->ResetCommandList();

        // If necessary, reallocate the memory for AABBs and the brick pool texture
        ray_tracer_->GetAccelerationStructure()->AllocateAABBBuffer(bricks_count_);
        AllocateBrickPoolTexture();

        // Fill AABB buffer with AABBs

        command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(surface_counts_buffer_.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ));
        command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(surface_cell_indices_buffer_.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ));
        command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(ray_tracer_->GetAccelerationStructure()->GetAABBBuffer(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

        command_list->SetPipelineState(compute_AABBs_state_object_.Get());
        command_list->SetComputeRootSignature(compute_AABBs_root_signature_.Get());
        command_list->SetComputeRootShaderResourceView(ComputeAABBsRootSignatureParams::SurfaceCellIndicesSlot, surface_cell_indices_buffer_->GetGPUVirtualAddress());
        command_list->SetComputeRootShaderResourceView(ComputeAABBsRootSignatureParams::SurfaceCountsSlot, surface_counts_buffer_->GetGPUVirtualAddress());
        command_list->SetComputeRootUnorderedAccessView(ComputeAABBsRootSignatureParams::AABBBufferSlot, ray_tracer_->GetAccelerationStructure()->GetAABBBuffer()->GetGPUVirtualAddress());
        command_list->SetComputeRootConstantBufferView(ComputeAABBsRootSignatureParams::TestValuesSlot, test_vals_cb_->Resource()->GetGPUVirtualAddress());
        command_list->Dispatch(std::ceil(bricks_count_ / 1024.f), 1, 1);

        command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(surface_counts_buffer_.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
        
        command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(ray_tracer_->GetAccelerationStructure()->GetAABBBuffer(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ));

        // Execute and wait for work to finish 
        device_resources_->ExecuteCommandList();
        device_resources_->WaitForGpu();
    }
}

void Computer::ReadBackCellCount()
{

    // Execute and wait for grid compute to finish 
    device_resources_->ExecuteCommandList();
    device_resources_->WaitForGpu();
    device_resources_->ResetCommandList();

    auto command_list = device_resources_->GetCommandList();

    // Schedule to copy the data to the default buffer to the readback buffer.
    command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(surface_counts_buffer_.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE));
    command_list->CopyResource(surface_counts_readback_buffer_.Get(), surface_counts_buffer_.Get());
    command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(surface_counts_buffer_.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

    // Execute and wait for grid compute to finish 
    device_resources_->ExecuteCommandList();
    device_resources_->WaitForGpu();


    // Map the data so we can read it on CPU.
    GridSurfaceCounts* mapped_data = nullptr;
    ThrowIfFailed(surface_counts_readback_buffer_->Map(0, nullptr, reinterpret_cast<void**>(&mapped_data)));

   /* std::wstring count = std::to_wstring(mapped_data->surface_cells);
    OutputDebugString(L"\ncell count:");
    OutputDebugString(count.c_str());*/

    //if (mapped_data->surface_cells > 0) {
        bricks_count_ = mapped_data->surface_cells * BRICKS_PER_CELL;
    //}

    surface_counts_readback_buffer_->Unmap(0, nullptr);
}

void Computer::ReadBackBlocksCount()
{
    // Execute and wait for grid compute to finish 
    device_resources_->ExecuteCommandList();
    device_resources_->WaitForGpu();
    device_resources_->ResetCommandList();

    //OutputDebugString(L"\nCOMPLETED 1ST COMPUTE\n");
    
    auto command_list = device_resources_->GetCommandList();

    // Schedule to copy the data to the default buffer to the readback buffer.
    command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(surface_counts_buffer_.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE));
    command_list->CopyResource(surface_counts_readback_buffer_.Get(), surface_counts_buffer_.Get());
    command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(surface_counts_buffer_.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
   
    // Execute and wait for grid compute to finish 
    device_resources_->ExecuteCommandList();
    device_resources_->WaitForGpu();
    device_resources_->ResetCommandList();


    // Map the data so we can read it on CPU.
    GridSurfaceCounts* mapped_data = nullptr;
    ThrowIfFailed(surface_counts_readback_buffer_->Map(0, nullptr, reinterpret_cast<void**>(&mapped_data)));

    /*std::wstring count = std::to_wstring(mapped_data->surface_blocks);
    OutputDebugString(L"\nblocks count:");
    OutputDebugString(count.c_str());*/

    //if (mapped_data->surface_blocks > 0) {
        surface_blocks_count_ = mapped_data->surface_blocks;
    //}

    surface_counts_readback_buffer_->Unmap(0, nullptr);
}


void Computer::ComputeBrickPoolTexture()
{
    auto command_list = device_resources_->GetCommandList();

    ID3D12DescriptorHeap* heap = application_->GetDescriptorHeap();
    command_list->SetDescriptorHeaps(1, &heap);

    command_list->SetPipelineState(compute_brickpool_state_object_.Get());
    command_list->SetComputeRootSignature(compute_brickpool_root_signature_.Get());

    command_list->SetComputeRootDescriptorTable(ComputeBrickPoolRootSignatureParams::TextureSlot, brick_pool_3d_texture_gpu_handle_);
    command_list->SetComputeRootShaderResourceView(ComputeBrickPoolRootSignatureParams::ParticlePositionsBufferSlot, particle_buffer_ordered_->GetGPUVirtualAddress());
    command_list->SetComputeRootShaderResourceView(ComputeBrickPoolRootSignatureParams::AABBBufferSlot, ray_tracer_->GetAccelerationStructure()->GetAABBBuffer()->GetGPUVirtualAddress());
    command_list->SetComputeRootShaderResourceView(ComputeBrickPoolRootSignatureParams::CellCountsSlot, scan_shader_->GetScanInBuffer()->GetGPUVirtualAddress());
    command_list->SetComputeRootShaderResourceView(ComputeBrickPoolRootSignatureParams::CellGlobalIndicexOffsetsSlot, scan_shader_->GetScanOutBuffer()->GetGPUVirtualAddress());
    command_list->SetComputeRootShaderResourceView(ComputeBrickPoolRootSignatureParams::SurfaceCellIndicesSlot, surface_cell_indices_buffer_->GetGPUVirtualAddress());
    command_list->SetComputeRootConstantBufferView(ComputeBrickPoolRootSignatureParams::ConstantBufferSlot, compute_cb_->Resource()->GetGPUVirtualAddress());
    command_list->SetComputeRootConstantBufferView(ComputeBrickPoolRootSignatureParams::TestValuesSlot, test_vals_cb_->Resource()->GetGPUVirtualAddress());

    command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(brick_pool_3d_texture_.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

    command_list->Dispatch(bricks_count_, 1, 1);

    command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(brick_pool_3d_texture_.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ));
    command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(surface_cell_indices_buffer_.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
}

void Computer::SortParticleData()
{
    // Scans the buffer containing particle counts for each cell, to determine global index offsets per cell to be used during reordering.
    scan_shader_->DispatchScan();

    auto command_list = device_resources_->GetCommandList();

    command_list->SetPipelineState(compute_reorder_state_object_.Get());
    command_list->SetComputeRootSignature(compute_reorder_root_signature_.Get());

    command_list->SetComputeRootUnorderedAccessView(ComputeReorderParticlesParams::OrderedParticlesSlot, particle_buffer_ordered_->GetGPUVirtualAddress());
    command_list->SetComputeRootShaderResourceView(ComputeReorderParticlesParams::UnorderedParticlesSlot, particle_buffer_unordered_->GetGPUVirtualAddress());
    command_list->SetComputeRootShaderResourceView(ComputeReorderParticlesParams::CellGlobalIndicexOffsetsSlot, scan_shader_->GetScanOutBuffer()->GetGPUVirtualAddress());
    command_list->SetComputeRootConstantBufferView(ComputeReorderParticlesParams::TestValuesSlot, test_vals_cb_->Resource()->GetGPUVirtualAddress());

    command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(particle_buffer_ordered_.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
    command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(particle_buffer_unordered_.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ));
    command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(scan_shader_->GetScanOutBuffer(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ));

    command_list->Dispatch(particle_threadgroups_, 1, 1);

    command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(particle_buffer_ordered_.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ));
    command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(particle_buffer_unordered_.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
    command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(scan_shader_->GetScanOutBuffer(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
}

void Computer::ComputeSimpleSDFTexture()
{
    auto command_list = device_resources_->GetCommandList();

    ID3D12DescriptorHeap* heap = application_->GetDescriptorHeap();
    command_list->SetDescriptorHeaps(1, &heap);

    command_list->SetPipelineState(compute_simple_tex_state_object_.Get());
    command_list->SetComputeRootSignature(compute_simple_tex_root_signature_.Get());

    command_list->SetComputeRootShaderResourceView(ComputeTextureRootSignatureParams::ParticlePositionsBufferSlot, particle_buffer_unordered_->GetGPUVirtualAddress());
    command_list->SetComputeRootDescriptorTable(ComputeTextureRootSignatureParams::TextureSlot, simple_sdf_3d_texture_gpu_handle_);
    command_list->SetComputeRootConstantBufferView(ComputeTextureRootSignatureParams::TestValuesSlot, test_vals_cb_->Resource()->GetGPUVirtualAddress());

    command_list->Dispatch(tex_creation_threadgroups_.x, tex_creation_threadgroups_.y, tex_creation_threadgroups_.z);

    command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(simple_sdf_3d_texture_.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ));
}

void Computer::CreateRootSignatures()
{
    // Root signature used for shader which computes particle positions
    CD3DX12_ROOT_PARAMETER pos_root_params[ComputePositionsRootSignatureParams::Count];
    pos_root_params[ComputePositionsRootSignatureParams::ParticlePositionsBufferSlot].InitAsUnorderedAccessView(0);
    pos_root_params[ComputePositionsRootSignatureParams::ConstantBufferSlot].InitAsConstantBufferView(1);
    pos_root_params[ComputePositionsRootSignatureParams::TestValuesSlot].InitAsConstantBufferView(0);
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
    grid_root_params[ComputeGridRootSignatureParams::TestValuesSlot].InitAsConstantBufferView(0);
    CD3DX12_ROOT_SIGNATURE_DESC grid_root_signature_desc(ARRAYSIZE(grid_root_params), grid_root_params);
    SerializeAndCreateComputeRootSignature(grid_root_signature_desc, &compute_grid_root_signature_);

    // Root signature used for building AABB buffer for acceleration structure
    CD3DX12_ROOT_PARAMETER build_AABBs_root_params[ComputeAABBsRootSignatureParams::Count];
    build_AABBs_root_params[ComputeAABBsRootSignatureParams::SurfaceCellIndicesSlot].InitAsShaderResourceView(0);
    build_AABBs_root_params[ComputeAABBsRootSignatureParams::SurfaceCountsSlot].InitAsShaderResourceView(1);
    build_AABBs_root_params[ComputeAABBsRootSignatureParams::AABBBufferSlot].InitAsUnorderedAccessView(0);
    build_AABBs_root_params[ComputeAABBsRootSignatureParams::TestValuesSlot].InitAsConstantBufferView(0);
    CD3DX12_ROOT_SIGNATURE_DESC build_AABBs_signature_desc(ARRAYSIZE(build_AABBs_root_params), build_AABBs_root_params);
    SerializeAndCreateComputeRootSignature(build_AABBs_signature_desc, &compute_AABBs_root_signature_);

    // Root signature used for shader which computes SDF texture
    CD3DX12_DESCRIPTOR_RANGE tex_uav_descriptor;
    tex_uav_descriptor.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
    CD3DX12_ROOT_PARAMETER tex_root_params[ComputeTextureRootSignatureParams::Count];
    tex_root_params[ComputeTextureRootSignatureParams::ParticlePositionsBufferSlot].InitAsShaderResourceView(0);
    tex_root_params[ComputeTextureRootSignatureParams::TestValuesSlot].InitAsConstantBufferView(0);
    tex_root_params[ComputeTextureRootSignatureParams::TextureSlot].InitAsDescriptorTable(1, &tex_uav_descriptor);
    CD3DX12_ROOT_SIGNATURE_DESC tex_root_signature_desc(ARRAYSIZE(tex_root_params), tex_root_params);
    SerializeAndCreateComputeRootSignature(tex_root_signature_desc, &compute_simple_tex_root_signature_);

    // Root signature used for shader which fills brick pool
    CD3DX12_DESCRIPTOR_RANGE brickpool_uav_descriptor;
    brickpool_uav_descriptor.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
    CD3DX12_ROOT_PARAMETER brickpool_root_params[ComputeBrickPoolRootSignatureParams::Count];
    brickpool_root_params[ComputeBrickPoolRootSignatureParams::TextureSlot].InitAsDescriptorTable(1, &brickpool_uav_descriptor);
    brickpool_root_params[ComputeBrickPoolRootSignatureParams::ParticlePositionsBufferSlot].InitAsShaderResourceView(0);
    brickpool_root_params[ComputeBrickPoolRootSignatureParams::AABBBufferSlot].InitAsShaderResourceView(1);
    brickpool_root_params[ComputeBrickPoolRootSignatureParams::CellCountsSlot].InitAsShaderResourceView(2);
    brickpool_root_params[ComputeBrickPoolRootSignatureParams::CellGlobalIndicexOffsetsSlot].InitAsShaderResourceView(3);
    brickpool_root_params[ComputeBrickPoolRootSignatureParams::SurfaceCellIndicesSlot].InitAsShaderResourceView(4);
    brickpool_root_params[ComputeBrickPoolRootSignatureParams::ConstantBufferSlot].InitAsConstantBufferView(1);
    brickpool_root_params[ComputeBrickPoolRootSignatureParams::TestValuesSlot].InitAsConstantBufferView(0);
    CD3DX12_ROOT_SIGNATURE_DESC brickpool_root_signature_desc(ARRAYSIZE(brickpool_root_params), brickpool_root_params);
    SerializeAndCreateComputeRootSignature(brickpool_root_signature_desc, &compute_brickpool_root_signature_);

    // Root signature used for shader which reorders particle data
    CD3DX12_ROOT_PARAMETER build_reorder_root_params[ComputeReorderParticlesParams::Count];
    build_reorder_root_params[ComputeReorderParticlesParams::OrderedParticlesSlot].InitAsUnorderedAccessView(0);
    build_reorder_root_params[ComputeReorderParticlesParams::UnorderedParticlesSlot].InitAsShaderResourceView(0);
    build_reorder_root_params[ComputeReorderParticlesParams::CellGlobalIndicexOffsetsSlot].InitAsShaderResourceView(1);
    build_reorder_root_params[ComputeReorderParticlesParams::TestValuesSlot].InitAsConstantBufferView(0);
    CD3DX12_ROOT_SIGNATURE_DESC build_reorder_signature_desc(ARRAYSIZE(build_reorder_root_params), build_reorder_root_params);
    SerializeAndCreateComputeRootSignature(build_reorder_signature_desc, &compute_reorder_root_signature_);


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

    // Generate particles shader
    if (FAILED(D3DCompileFromFile(application_->GetAssetFullPath(L"ComputePositions.hlsl").c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "CSParticleGen", "cs_5_1", flags, 0, &compute_shader, &error_blob))) {
        std::string errMsg((char*)error_blob->GetBufferPointer(), error_blob->GetBufferSize());
        throw std::exception(errMsg.c_str());
    }
    compute_pso.pRootSignature = compute_pos_root_signature_.Get();
    compute_pso.CS = CD3DX12_SHADER_BYTECODE(compute_shader.Get());
    ThrowIfFailed(device_resources_->GetD3DDevice()->CreateComputePipelineState(&compute_pso, IID_PPV_ARGS(&compute_generate_state_object_)));

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
    compute_pso.CS = CD3DX12_SHADER_BYTECODE(compute_shader.Get());
    ThrowIfFailed(device_resources_->GetD3DDevice()->CreateComputePipelineState(&compute_pso, IID_PPV_ARGS(&compute_surface_blocks_state_object_)));

    // Detect surface cells shader
    if (FAILED(D3DCompileFromFile(application_->GetAssetFullPath(L"ComputeGrid.hlsl").c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "CSDetectSurfaceCellsMain", "cs_5_1", flags, 0, &compute_shader, &error_blob))) {
        std::string errMsg((char*)error_blob->GetBufferPointer(), error_blob->GetBufferSize());
        throw std::exception(errMsg.c_str());
    }
    compute_pso.CS = CD3DX12_SHADER_BYTECODE(compute_shader.Get());
    ThrowIfFailed(device_resources_->GetD3DDevice()->CreateComputePipelineState(&compute_pso, IID_PPV_ARGS(&compute_surface_cells_state_object_)));

    // Build AABB buffer
    if (FAILED(D3DCompileFromFile(application_->GetAssetFullPath(L"ComputeBuildAABBs.hlsl").c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "CSBuildAABBs", "cs_5_1", flags, 0, &compute_shader, &error_blob))) {
        std::string errMsg((char*)error_blob->GetBufferPointer(), error_blob->GetBufferSize());
        throw std::exception(errMsg.c_str());
    }
    compute_pso.pRootSignature = compute_AABBs_root_signature_.Get();
    compute_pso.CS = CD3DX12_SHADER_BYTECODE(compute_shader.Get());
    ThrowIfFailed(device_resources_->GetD3DDevice()->CreateComputePipelineState(&compute_pso, IID_PPV_ARGS(&compute_AABBs_state_object_)));

    // Compute texture shader
    if (FAILED(D3DCompileFromFile(application_->GetAssetFullPath(L"ComputeTexture.hlsl").c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "CSTexMain", "cs_5_1", flags, 0, &compute_shader, &error_blob))) {
        std::string errMsg((char*)error_blob->GetBufferPointer(), error_blob->GetBufferSize());
        throw std::exception(errMsg.c_str());
    }
    compute_pso.pRootSignature = compute_simple_tex_root_signature_.Get();
    compute_pso.CS = CD3DX12_SHADER_BYTECODE(compute_shader.Get());
    ThrowIfFailed(device_resources_->GetD3DDevice()->CreateComputePipelineState(&compute_pso, IID_PPV_ARGS(&compute_simple_tex_state_object_)));

    // Compute brick pool shader
    if (FAILED(D3DCompileFromFile(application_->GetAssetFullPath(L"ComputeBrickPool.hlsl").c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "CSBrickPoolMain", "cs_5_1", flags, 0, &compute_shader, &error_blob))) {
        std::string errMsg((char*)error_blob->GetBufferPointer(), error_blob->GetBufferSize());
        throw std::exception(errMsg.c_str());
    }
    compute_pso.pRootSignature = compute_brickpool_root_signature_.Get();
    compute_pso.CS = CD3DX12_SHADER_BYTECODE(compute_shader.Get());
    ThrowIfFailed(device_resources_->GetD3DDevice()->CreateComputePipelineState(&compute_pso, IID_PPV_ARGS(&compute_brickpool_state_object_)));

    // Reorder particle data shader
    if (FAILED(D3DCompileFromFile(application_->GetAssetFullPath(L"ComputeReorderParticles.hlsl").c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "CSReorderParticlesMain", "cs_5_1", flags, 0, &compute_shader, &error_blob))) {
        std::string errMsg((char*)error_blob->GetBufferPointer(), error_blob->GetBufferSize());
        throw std::exception(errMsg.c_str());
    }
    compute_pso.pRootSignature = compute_reorder_root_signature_.Get();
    compute_pso.CS = CD3DX12_SHADER_BYTECODE(compute_shader.Get());
    ThrowIfFailed(device_resources_->GetD3DDevice()->CreateComputePipelineState(&compute_pso, IID_PPV_ARGS(&compute_reorder_state_object_)));

}

void Computer::CreateBuffers()
{
    auto command_list = device_resources_->GetCommandList();
    auto device = device_resources_->GetD3DDevice();
    UINT descriptor_size = device_resources_->GetD3DDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // Calculate threadgroup sizes
    particle_threadgroups_ = std::ceil(NUM_PARTICLES / 1024.f);
    blocks_threadgroups_ = std::ceil(NUM_BLOCKS / 1024.f);
    clear_counts_threadgroups_ = std::ceil(NUM_CELLS / 1024.f);
    tex_creation_threadgroups_ = XMUINT3(std::ceil(TEXTURE_RESOLUTION / 32.f), std::ceil(TEXTURE_RESOLUTION / 32.f), TEXTURE_RESOLUTION);

    // Particle position buffer
    //std::vector<ParticleData> positions;
    ////ParticleData positions[NUM_PARTICLES];
    //int bruv = NUM_PARTICLES;
    //for (int i = 0; i < NUM_PARTICLES; i++) {
    //    ParticleData p;
    //    p.position_.x = ((float)(rand() % 10) + 0.5f) / 10.f;
    //    p.position_.y = ((float)(rand() % 10) + 0.5f) / 10.f;
    //    p.position_.z = ((float)(rand() % 10) + 0.5f) / 10.f;
    //    p.speed_ = rand() % 10 + 1;
    //    p.start_y_ = p.position_.y;

    //    positions.push_back(p);
    //}

   /* for (int i = 0; i < NUM_PARTICLES; i++) {
        positions[i].position_.x = i * 0.0625 + 0.15f;
        positions[i].position_.y = 0.25f;
        positions[i].position_.z = 0.15f;
        positions[i].speed_ = rand() % 10 + 1;
        positions[i].start_y_ = positions[i].position_.y;
    }*/

    /*positions[0].position_.x = 0.05f;
    positions[0].position_.y = 0.251f;
    positions[0].position_.z = 0.05f;
    positions[0].speed_ = 0;
    positions[0].start_y_ = positions[0].position_.y;*/

    /*positions[1].position_.x = 0.05f;
    positions[1].position_.y = 0.05f;VOXELS_PER_AXIS_PER_BRICK
    positions[1].position_.z = 0.05f;
    positions[1].speed_ = 0;
    positions[1].start_y_ = positions[0].position_.y;*/



    UINT64 byte_size = NUM_PARTICLES * sizeof(ParticleData);

    //particle_buffer_unordered_ = Utilities::CreateDefaultBuffer(device, command_list, positions.data(), byte_size, particle_buffer_uploader_, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    Utilities::AllocateDefaultBuffer(device, byte_size, particle_buffer_unordered_.GetAddressOf(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    Utilities::AllocateDefaultBuffer(device, byte_size, particle_buffer_ordered_.GetAddressOf(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    particle_buffer_unordered_->SetName(L"UnorderedP");
    particle_buffer_ordered_->SetName(L"OrderedP");

    // Grid buffers
    //Utilities::AllocateDefaultBuffer(device, NUM_CELLS * sizeof(Cell), cells_buffer_.GetAddressOf(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    scan_shader_ = std::make_unique<ChainedScanDecoupledLookback>(device_resources_);
    scan_shader_->UpdateSize(NUM_CELLS);
    Utilities::AllocateDefaultBuffer(device, NUM_BLOCKS * sizeof(Block), blocks_buffer_.GetAddressOf(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    Utilities::AllocateDefaultBuffer(device, NUM_CELLS * sizeof(unsigned int), surface_cell_indices_buffer_.GetAddressOf(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    Utilities::AllocateDefaultBuffer(device, NUM_BLOCKS * sizeof(unsigned int), surface_block_indices_buffer_.GetAddressOf(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    Utilities::AllocateDefaultBuffer(device, sizeof(GridSurfaceCounts), surface_counts_buffer_.GetAddressOf(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    blocks_buffer_->SetName(L"Blocks");
    surface_cell_indices_buffer_->SetName(L"SurfaceCellIndices");
    surface_block_indices_buffer_->SetName(L"SurfaceBlockIndices");
    surface_counts_buffer_->SetName(L"SurfaceCounts");




    // Allocate buffer for reading back surface cell count
    ThrowIfFailed(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(sizeof(GridSurfaceCounts)),
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&surface_counts_readback_buffer_)));
}

void Computer::AllocateBrickPoolTexture()
{
    // If count of surface cells is larger than max bricks the pool can store
    if (bricks_count_ > max_bricks_count_) {
        XMUINT3 dimensions;
        max_bricks_count_ = FindOptimalBrickPoolDimensions(dimensions); 

        // Release the texture
        brick_pool_3d_texture_.Reset();

        // Create the 3D texture 
        auto uavDesc = CD3DX12_RESOURCE_DESC::Tex3D(DXGI_FORMAT_R16_SNORM, dimensions.x * VOXELS_PER_AXIS_PER_BRICK, dimensions.y * VOXELS_PER_AXIS_PER_BRICK, dimensions.z * VOXELS_PER_AXIS_PER_BRICK, 1, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

        auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        ThrowIfFailed(device_resources_->GetD3DDevice()->CreateCommittedResource(
            &defaultHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &uavDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr, IID_PPV_ARGS(&brick_pool_3d_texture_)));

        device_resources_->GetD3DDevice()->CreateUnorderedAccessView(brick_pool_3d_texture_.Get(), nullptr, nullptr, brick_pool_3d_texture_cpu_handle_);

        // Upload dimensions for use in texture creation
        compute_cb_->Values().brick_pool_dimensions_ = std::move(dimensions);
        compute_cb_->CopyData(0);
    }
}

// Approximates optimal dimensions for the brick pool
unsigned int Computer::FindOptimalBrickPoolDimensions(XMUINT3& dimensions)
{
    // Greedily assign the smallest possible value, >= the cube root, to the x dimension
    int cube_root = std::ceil(std::cbrt(bricks_count_)); 
    dimensions.x = cube_root;

    // Iteratively adjust dimensions till a suitable solution is found
    while (dimensions.x > 0) {
        int remaining = std::ceil((double)bricks_count_ / dimensions.x);
        int y_approx = std::sqrt(remaining);
        int z_approx = std::ceil((double)remaining / y_approx);

        if (dimensions.x * y_approx * z_approx >= bricks_count_) {
            dimensions.y = y_approx;
            dimensions.z = z_approx;
            return dimensions.x * dimensions.y * dimensions.z;
        }

        dimensions.x--;
    }

    // No solution found, allocate according to cube root
    dimensions.x = dimensions.y = dimensions.z = cube_root;

    return dimensions.x * dimensions.y * dimensions.z;
}

void Computer::AllocateSimpleSDFTexture()
{
    // Create the 3D texture 
    auto uavDesc = CD3DX12_RESOURCE_DESC::Tex3D(DXGI_FORMAT_R16_SNORM, TEXTURE_RESOLUTION, TEXTURE_RESOLUTION, TEXTURE_RESOLUTION, 1, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    ThrowIfFailed(device_resources_->GetD3DDevice()->CreateCommittedResource(
        &defaultHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &uavDesc,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        nullptr, IID_PPV_ARGS(&simple_sdf_3d_texture_)));

    // Allocate descriptors
    D3D12_CPU_DESCRIPTOR_HANDLE uav_handle;
    UINT heap_index = application_->AllocateDescriptor(&uav_handle);
    device_resources_->GetD3DDevice()->CreateUnorderedAccessView(simple_sdf_3d_texture_.Get(), nullptr, nullptr, uav_handle);


    UINT descriptor_size = device_resources_->GetD3DDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    simple_sdf_3d_texture_gpu_handle_ = CD3DX12_GPU_DESCRIPTOR_HANDLE(application_->GetDescriptorHeap()->GetGPUDescriptorHandleForHeapStart(), heap_index, descriptor_size);

    // Also allocate descriptors for the brick pool
    heap_index = application_->AllocateDescriptor(&brick_pool_3d_texture_cpu_handle_);
    brick_pool_3d_texture_gpu_handle_ = CD3DX12_GPU_DESCRIPTOR_HANDLE(application_->GetDescriptorHeap()->GetGPUDescriptorHandleForHeapStart(), heap_index, descriptor_size);
}

