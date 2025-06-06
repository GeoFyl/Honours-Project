#pragma once
#include "RayTracingStructs.h"
#include "DeviceResources.h"
#include "ShaderTable.h"
#include "Computer.h"
#include "AccelerationStructureManager.h"
#include <dxcapi.h>

namespace GlobalRTRootSignatureParams {
    enum Value {
        OutputViewSlot = 0,
        AccelerationStructureSlot,
        RTConstantBufferSlot,
        CompConstantBufferSlot,
        ParticlePositionsBufferSlot,
        SDFTextureSlot,
        AABBBufferSlot,
        TestValuesSlot,
        Count
    };
}

using Microsoft::WRL::ComPtr;

class HonoursApplication;

class RayTracer
{
public:
	RayTracer(DX::DeviceResources* device_resources, HonoursApplication* app, Computer* comp);

    void RayTracing(Profiler* profiler);

    void CreateRaytracingOutputResource();

    inline AccelerationStructureManager* GetAccelerationStructure() { return acceleration_structure_.get(); }
    inline UploadBuffer<RayTracingCB>* GetRaytracingCB() { return ray_tracing_cb_.get(); }
    inline ID3D12Resource* GetRaytracingOutput() { return m_raytracingOutput.Get(); }
    inline void ReleaseUploaders() { aabb_buffer_uploader_.Reset(); }

	static void CheckRayTracingSupport(ID3D12Device5* device);

private:
    void CreateRootSignatures();
    void SerializeAndCreateRaytracingRootSignature(D3D12_ROOT_SIGNATURE_DESC& desc, ComPtr<ID3D12RootSignature>* rootSig);
    void CreateRaytracingPipelineStateObject();

    void BuildSimpleAccelerationStructure();

    void BuildShaderTables();
    IDxcBlob* CompileShaderLibrary(LPCWSTR fileName);

    // DXR attributes
    ComPtr<ID3D12StateObject> rt_state_object_;

    // Root signatures
    ComPtr<ID3D12RootSignature> rt_global_root_signature_;

    // Simple acceleration structure for naive implementation
    ComPtr<ID3D12Resource> top_simple_acceleration_structure;
    ComPtr<ID3D12Resource> bottom_simple_acceleration_structure;
    ComPtr<ID3D12Resource> simple_aabb_buffer_;
    ComPtr<ID3D12Resource> aabb_buffer_uploader_;

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

    // Constant buffer
    std::unique_ptr<UploadBuffer<RayTracingCB>> ray_tracing_cb_ = nullptr;

    // Application stuff
    std::unique_ptr<AccelerationStructureManager> acceleration_structure_ = nullptr;
    DX::DeviceResources* device_resources_;
    HonoursApplication* application_;
    Computer* computer_;
};

