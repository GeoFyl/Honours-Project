#pragma once
#include "DeviceResources.h"
#include "ComputeStructs.h"

namespace ComputePositionsRootSignatureParams {
    enum Value {
        ParticlePositionsBufferSlot = 0,
        ConstantBufferSlot,
        Count
    };
}
namespace ComputeTextureRootSignatureParams {
    enum Value {
        ParticlePositionsBufferSlot = 0,
        TextureSlot,
        Count
    };
}
namespace ComputeGridRootSignatureParams {
    enum Value {
        ParticlePositionsBufferSlot = 0,
        CellsSlot,
        BlocksSlot,
        SurfaceBlocksSlot,
        SurfaceCellsSlot,
        SurfaceCountsSlot,
        Count
    };
}
namespace ComputeDispatchCellsRootSignatureParams {
    enum Value {
        SurfaceCountsSlot = 0,
        DispatchArgsSlot,
        Count
    };
}


using Microsoft::WRL::ComPtr;

class HonoursApplication;

class Computer
{
public: 
    Computer(DX::DeviceResources* device_resources, HonoursApplication* app);

    void ComputePostitions();
    void ComputeGrid();
    void ComputeSDFTexture();
    void ReadBackCellCount();

    inline ID3D12Resource* GetPositionsBuffer() { return particle_pos_buffer_.Get(); }
    //inline ID3D12Resource* GetAABBBuffer() { return aabb_buffer_.Get(); }
    inline ID3D12Resource* GetSDFTexture() { return sdf_3d_texture_.Get(); }
    inline D3D12_GPU_DESCRIPTOR_HANDLE GetSDFTextureHandle() { return sdf_3d_texture_gpu_handle_; }

    inline void ReleaseUploaders() {
        particle_pos_buffer_uploader_.Reset();
    }

private:

    void CreateRootSignatures();
    void SerializeAndCreateComputeRootSignature(D3D12_ROOT_SIGNATURE_DESC& desc, ComPtr<ID3D12RootSignature>* rootSig);
    void CreateComputePipelineStateObjects();
    void CreateBuffers();
    void CreateTexture3D();

    // DXR attributes
    ComPtr<ID3D12PipelineState> compute_pos_state_object_;
    ComPtr<ID3D12PipelineState> compute_grid_state_object_;
    ComPtr<ID3D12PipelineState> compute_clear_counts_state_object_;
    ComPtr<ID3D12PipelineState> compute_surface_blocks_state_object_;
    ComPtr<ID3D12PipelineState> compute_dispatch_surface_cells_state_object_;
    ComPtr<ID3D12PipelineState> compute_surface_cells_state_object_;
    ComPtr<ID3D12PipelineState> compute_tex_state_object_;

    // Root signatures
    ComPtr<ID3D12RootSignature> compute_pos_root_signature_;
    ComPtr<ID3D12RootSignature> compute_grid_root_signature_;
    ComPtr<ID3D12RootSignature> compute_dispatch_surface_cells_root_signature_;
    //ComPtr<ID3D12RootSignature> compute_surface_blocks_root_signature_;
    //ComPtr<ID3D12RootSignature> compute_surface_cells_root_signature_;
    ComPtr<ID3D12RootSignature> compute_tex_root_signature_;
    
    ComPtr<ID3D12CommandSignature> compute_surface_cells_command_signature_;

    // Buffers
    ComPtr<ID3D12Resource> particle_pos_buffer_uploader_;
    ComPtr<ID3D12Resource> particle_pos_buffer_;

    ComPtr<ID3D12Resource> cells_buffer_;
    ComPtr<ID3D12Resource> blocks_buffer_;
    ComPtr<ID3D12Resource> surface_block_indices_buffer_;
    ComPtr<ID3D12Resource> surface_cell_indices_buffer_;
    ComPtr<ID3D12Resource> surface_counts_buffer_;
    ComPtr<ID3D12Resource> surface_counts_readback_buffer_;

    ComPtr<ID3D12Resource> surface_cells_dispatch_buffer_uploader_;
    ComPtr<ID3D12Resource> surface_cells_dispatch_buffer_;

    //ComPtr<ID3D12Resource> aabb_buffer_uploader_;
    //ComPtr<ID3D12Resource> aabb_buffer_;



    // 3D texture
    ComPtr<ID3D12Resource> sdf_3d_texture_;
    D3D12_GPU_DESCRIPTOR_HANDLE sdf_3d_texture_gpu_handle_;

    // threadgroup sizes
    UINT particle_threadgroups_; // for particle position manipulation shader
    UINT blocks_threadgroups_; // for surface block detection
    UINT clear_counts_threadgroups_; // for clearing counters
    XMUINT3 tex_creation_threadgroups_; // for texture creation shader

    DX::DeviceResources* device_resources_;
    HonoursApplication* application_;
};

