#pragma once
#include "RayTracingStructs.h"
#include "DeviceResources.h"
#include "ShaderTable.h"
#include "Computer.h"
#include <dxcapi.h>

namespace GlobalRTRootSignatureParams {
    enum Value {
        OutputViewSlot = 0,
        AccelerationStructureSlot,
        ConstantBufferSlot,
        ParticlePositionsBufferSlot,
        SDFTextureSlot,
        Count
    };
}

namespace LocalRootSignatureParams {
    enum Value {
        AABBBufferSlot = 0,
        Count
    };
}

using Microsoft::WRL::ComPtr;

class HonoursApplication;

class RayTracer
{
public:
	RayTracer(DX::DeviceResources* device_resources, HonoursApplication* app, Computer* comp);

    void RayTracing();

    void CreateRaytracingOutputResource();

    inline ID3D12Resource* GetRaytracingOutput() { return m_raytracingOutput.Get(); }
    inline void ReleaseUploaders() { aabb_buffer_uploader_.Reset(); }

	static void CheckRayTracingSupport(ID3D12Device5* device);

private:
    void CreateRootSignatures();
    void SerializeAndCreateRaytracingRootSignature(D3D12_ROOT_SIGNATURE_DESC& desc, ComPtr<ID3D12RootSignature>* rootSig);
    void CreateRaytracingPipelineStateObject();

    void BuildAccelerationStructures();

    void BuildShaderTables();
    IDxcBlob* CompileShaderLibrary(LPCWSTR fileName);

    // DXR attributes
    ComPtr<ID3D12StateObject> rt_state_object_;

    // Root signatures
    ComPtr<ID3D12RootSignature> rt_global_root_signature_;
    ComPtr<ID3D12RootSignature> rt_hit_local_root_signature_;

    // Acceleration structure
    ComPtr<ID3D12Resource> m_accelerationStructure;
    ComPtr<ID3D12Resource> m_bottomLevelAccelerationStructure;
    ComPtr<ID3D12Resource> m_topLevelAccelerationStructure;

    ComPtr<ID3D12Resource> aabb_buffer_uploader_;
    ComPtr<ID3D12Resource> aabb_buffer_;

    // Raytracing output
    ComPtr<ID3D12Resource> m_raytracingOutput;
    D3D12_GPU_DESCRIPTOR_HANDLE m_raytracingOutputResourceUAVGpuDescriptor;
    UINT m_raytracingOutputResourceUAVDescriptorHeapIndex = -1;
    XMFLOAT2 window_size_;

    // Shader tables
    static const wchar_t* hit_group_name_;
    static const wchar_t* ray_gen_shader_name_;
    static const wchar_t* intersection_shader_name_;
    static const wchar_t* closest_hit_shader_name_;
    static const wchar_t* miss_shader_name;
    std::unique_ptr<ShaderTable> m_missShaderTable;
    std::unique_ptr<ShaderTable> m_hitGroupShaderTable;
    std::unique_ptr<ShaderTable> m_rayGenShaderTable;

    DX::DeviceResources* device_resources_;
    HonoursApplication* application_;
    Computer* computer_;
    //ID3D12Device5* device_ = nullptr;
    //ID3D12GraphicsCommandList4* command_list_ = nullptr;
};

