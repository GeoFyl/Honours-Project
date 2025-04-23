#pragma once
#include <comdef.h>
#include <d3d12.h>
#include "DeviceResources.h"
#include "UploadBuffer.h"
#include "Profiler.h"

using Microsoft::WRL::ComPtr;

class AccelerationStructureManager
{
public:
    AccelerationStructureManager(DX::DeviceResources* device_resources);
    void AllocateAABBBuffer(int new_aabb_count);
    ID3D12Resource* GetAABBBuffer() { return aabb_buffer_.Get(); }

    void UpdateStructure(Profiler* profiler);
    ID3D12Resource* GetTLAS() { return top_acceleration_structure_.Get(); }
    ID3D12Resource* GetBLAS() { return bottom_acceleration_structure_.Get(); }
    bool IsStructureBuilt() { return structure_built; }

private:
    void CalculatePreBuildInfo(D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& blas_inputs, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO& blas_prebuild_info);
    void RebuildStructure(D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& blas_inputs, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO& blas_prebuild_info, Profiler* profiler);
    void BuildStructures(D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& blas_inputs, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO& blas_prebuild_info, Profiler* profiler, bool update = false);

    // Acceleration buffers
    ComPtr<ID3D12Resource> bottom_acceleration_structure_;
    ComPtr<ID3D12Resource> top_acceleration_structure_;
    ComPtr<ID3D12Resource> scratch_resource_;

    // Info used in acceleration structure construction
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO top_level_prebuild_info_;
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS top_level_inputs_;
    std::unique_ptr<UploadBuffer<D3D12_RAYTRACING_INSTANCE_DESC>> instance_desc_buffer;
    D3D12_RAYTRACING_GEOMETRY_DESC geometry_desc_;

    // Buffer for AABBs used for BLAS construction
    ComPtr<ID3D12Resource> aabb_buffer_;

    // Misc
    unsigned int aabb_count_ = 0;
    unsigned int max_aabb_count_ = 0;
    bool requires_rebuild_ = false;
    bool structure_built = false;

    DX::DeviceResources* device_resources_;
};