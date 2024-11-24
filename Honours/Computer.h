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


using Microsoft::WRL::ComPtr;

class HonoursApplication;

class Computer
{
public: 
    Computer(DX::DeviceResources* device_resources, HonoursApplication* app);

    void ComputePostitions();
    void ComputeSDFTexture();

    inline ID3D12Resource* GetPositionsBuffer() { return particle_pos_buffer_.Get(); }
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
    ComPtr<ID3D12PipelineState> compute_tex_state_object_;

    // Root signatures
    ComPtr<ID3D12RootSignature> compute_pos_root_signature_;
    ComPtr<ID3D12RootSignature> compute_tex_root_signature_;

    ComPtr<ID3D12Resource> particle_pos_buffer_uploader_;
    ComPtr<ID3D12Resource> particle_pos_buffer_;

    // 3D texture
    ComPtr<ID3D12Resource> sdf_3d_texture_;
    D3D12_GPU_DESCRIPTOR_HANDLE sdf_3d_texture_gpu_handle_;

    // threadgroup size for particle position manipulation shader
    UINT particle_pos_manip_threadgroups_ = 1;
    // threadgroup size for texture creation shader
    XMUINT3 tex_creation_threadgroups_ = XMUINT3(1, 1, 1);

    DX::DeviceResources* device_resources_;
    HonoursApplication* application_;
};

