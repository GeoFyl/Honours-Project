#pragma once
#include "DeviceResources.h"
#include "ComputeStructs.h"

#define NUM_PARTICLES 20

namespace GlobalComputeRootSignatureParams {
    enum Value {
        ParticlePositionsConsumeBufferSlot = 0,
        ParticlePositionsAppendBufferSlot = 0,
        Count
    };
}

using Microsoft::WRL::ComPtr;

class HonoursApplication;

class Computer
{
public: 
    Computer(DX::DeviceResources* device_resources, HonoursApplication* app);

    void Compute();

    inline ID3D12Resource* GetOutputBuffer() { return consume_buffer_index_ ? particle_pos_buffer_0_.Get() : particle_pos_buffer_1_.Get(); }

    inline void ReleaseUploaders() {
        particle_pos_buffer_uploader_.Reset();
    }

private:

    void CreateRootSignatures();
    void SerializeAndCreateComputeRootSignature(D3D12_ROOT_SIGNATURE_DESC& desc, ComPtr<ID3D12RootSignature>* rootSig);
    void CreateComputePipelineStateObject();
    void CreateBuffers();

    // DXR attributes
    ComPtr<ID3D12PipelineState> compute_state_object_;

    // Root signatures
    ComPtr<ID3D12RootSignature> compute_global_root_signature_;

    ComPtr<ID3D12Resource> particle_pos_buffer_uploader_;
    ComPtr<ID3D12Resource> particle_pos_buffer_0_;
    ComPtr<ID3D12Resource> particle_pos_buffer_1_;

    D3D12_GPU_DESCRIPTOR_HANDLE particle_pos_buffer_0_gpu_handle_;
    D3D12_GPU_DESCRIPTOR_HANDLE particle_pos_buffer_1_gpu_handle_;

    ComPtr<ID3D12Resource> particle_pos_buffer_0_counter_;
    ComPtr<ID3D12Resource> particle_pos_buffer_1_counter_;

    UINT consume_buffer_index_ = 1;

    DX::DeviceResources* device_resources_;
    HonoursApplication* application_;
};

