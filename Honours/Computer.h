#pragma once
#include "DeviceResources.h"
#include "ComputeStructs.h"
#include "UploadBuffer.h"
#include "ChainedScanDecoupledLookback.h"

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
namespace ComputeBrickPoolRootSignatureParams {
    enum Value {
        ParticlePositionsBufferSlot = 0,
        TextureSlot,
        AABBBufferSlot,
        ConstantBufferSlot,
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
namespace ComputeAABBsRootSignatureParams {
    enum Value {
        SurfaceCellIndicesSlot = 0,
        SurfaceCountsSlot,
        AABBBufferSlot,
        Count
    };
}


using Microsoft::WRL::ComPtr;

class HonoursApplication;
class RayTracer;

class Computer
{
public: 
    Computer(DX::DeviceResources* device_resources, HonoursApplication* app);
    void SetRayTracer(RayTracer* ray_tracer) { ray_tracer_ = ray_tracer; }

    void ComputePostitions();
    void ComputeGrid();
    void ComputeSimpleSDFTexture();
    void ReadBackCellCount();
    void ComputeAABBs();
    void ComputeBrickPoolTexture();


    //UINT GetSurfaceCellCount() { return surface_cell_count_; }

    inline UploadBuffer<ComputeCB>* GetConstantBuffer() { return compute_cb_.get(); }
    inline ID3D12Resource* GetPositionsBuffer() { return particle_pos_buffer_.Get(); }
    inline ID3D12Resource* GetSimpleSDFTexture() { return simple_sdf_3d_texture_.Get(); }
    inline D3D12_GPU_DESCRIPTOR_HANDLE GetSimpleSDFTextureHandle() { return simple_sdf_3d_texture_gpu_handle_; }
    inline ID3D12Resource* GetBrickPoolTexture() { return brick_pool_3d_texture_.Get(); }
    inline D3D12_GPU_DESCRIPTOR_HANDLE GetBrickPoolTextureHandle() { return brick_pool_3d_texture_gpu_handle_; }

    inline void ReleaseUploaders() {
        particle_pos_buffer_uploader_.Reset();
    }

private:

    void CreateRootSignatures();
    void SerializeAndCreateComputeRootSignature(D3D12_ROOT_SIGNATURE_DESC& desc, ComPtr<ID3D12RootSignature>* rootSig);
    void CreateComputePipelineStateObjects();
    void CreateBuffers();
    void AllocateSimpleSDFTexture();
    void ReadBackBlocksCount();
    void AllocateBrickPoolTexture();
    UINT FindOptimalBrickPoolDimensions(XMUINT3& dimensions);

    // Implementation of chained scan with decoupled lookback from https://github.com/b0nes164/GPUPrefixSums
    std::unique_ptr<ChainedScanDecoupledLookback> scan_shader_;

    // State Objects
    ComPtr<ID3D12PipelineState> compute_pos_state_object_;
    ComPtr<ID3D12PipelineState> compute_grid_state_object_;
    ComPtr<ID3D12PipelineState> compute_clear_counts_state_object_;
    ComPtr<ID3D12PipelineState> compute_surface_blocks_state_object_;
    ComPtr<ID3D12PipelineState> compute_surface_cells_state_object_;
    ComPtr<ID3D12PipelineState> compute_AABBs_state_object_;
    ComPtr<ID3D12PipelineState> compute_simple_tex_state_object_;
    ComPtr<ID3D12PipelineState> compute_brickpool_state_object_;

    // Root signatures
    ComPtr<ID3D12RootSignature> compute_pos_root_signature_;
    ComPtr<ID3D12RootSignature> compute_grid_root_signature_;
    ComPtr<ID3D12RootSignature> compute_AABBs_root_signature_;
    ComPtr<ID3D12RootSignature> compute_simple_tex_root_signature_;
    ComPtr<ID3D12RootSignature> compute_brickpool_root_signature_;

    // Buffers
    ComPtr<ID3D12Resource> particle_pos_buffer_uploader_;
    ComPtr<ID3D12Resource> particle_pos_buffer_;

    //ComPtr<ID3D12Resource> cells_buffer_;
    ComPtr<ID3D12Resource> blocks_buffer_;
    ComPtr<ID3D12Resource> surface_block_indices_buffer_;
    ComPtr<ID3D12Resource> surface_cell_indices_buffer_;
    ComPtr<ID3D12Resource> surface_counts_buffer_;
    ComPtr<ID3D12Resource> surface_counts_readback_buffer_;

    std::unique_ptr<UploadBuffer<ComputeCB>> compute_cb_ = nullptr;

    // Values
    UINT surface_blocks_count_ = 0;
    UINT bricks_count_ = 0;
    UINT max_bricks_count_ = 0;

    // 3D texture
    ComPtr<ID3D12Resource> simple_sdf_3d_texture_;
    D3D12_GPU_DESCRIPTOR_HANDLE simple_sdf_3d_texture_gpu_handle_;
    ComPtr<ID3D12Resource> brick_pool_3d_texture_;
    D3D12_GPU_DESCRIPTOR_HANDLE brick_pool_3d_texture_gpu_handle_;
    D3D12_CPU_DESCRIPTOR_HANDLE brick_pool_3d_texture_cpu_handle_;
   // UINT brick_pool_3d_texture_heap_index_;
    //bool brick_pool_allocated_ = false;

    // threadgroup sizes
    UINT particle_threadgroups_; // for particle position manipulation shader
    UINT blocks_threadgroups_; // for surface block detection
    UINT clear_counts_threadgroups_; // for clearing counters
    XMUINT3 tex_creation_threadgroups_; // for simple texture creation shader
    XMUINT3 brickpool_creation_threadgroups_; // for brick pool texture creation shader

    DX::DeviceResources* device_resources_;
    HonoursApplication* application_;
    RayTracer* ray_tracer_;
};

