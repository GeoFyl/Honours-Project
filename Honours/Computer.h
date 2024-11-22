#pragma once
#include "DeviceResources.h"
#include "ComputeStructs.h"

namespace GlobalComputeRootSignatureParams {
    enum Value {
        ParticlePositionsBufferSlot = 0,
        ConstantBufferSlot,
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

    inline ID3D12Resource* GetOutputBuffer() { return particle_pos_buffer_.Get(); }

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
    ComPtr<ID3D12Resource> particle_pos_buffer_;
    UINT threadgroups_ = 1;

    DX::DeviceResources* device_resources_;
    HonoursApplication* application_;
};

