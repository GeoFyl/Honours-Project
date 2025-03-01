#pragma once
#include <comdef.h>
#include <d3d12.h>
#include "DeviceResources.h"
#include "UploadBuffer.h"

using Microsoft::WRL::ComPtr;

class AccelerationStructureManager
{
public:
    AccelerationStructureManager(DX::DeviceResources* device_resources);
    void AllocateAABBBuffer(int new_aabb_count);
    ID3D12Resource* GetAABBBuffer() { return aabb_buffer_.Get(); }

    void UpdateStructure();
    ID3D12Resource* GetStructure() { return top_acceleration_structure_.Get(); }

private:
    void CalculatePreBuildInfo(D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& blas_inputs, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO& blas_prebuild_info);
    void RebuildStructure(D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& blas_inputs, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO& blas_prebuild_info);
    void BuildStructures(D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& blas_inputs, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO& blas_prebuild_info, bool update = false);


    // Acceleration buffers
    ComPtr<ID3D12Resource> bottom_acceleration_structure_;
    ComPtr<ID3D12Resource> top_acceleration_structure_;
    ComPtr<ID3D12Resource> scratch_resource_;

    // Info used in aceceleration structure construction
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO top_level_prebuild_info_;
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS top_level_inputs_;
    std::unique_ptr<UploadBuffer<D3D12_RAYTRACING_INSTANCE_DESC>> instance_desc_buffer;
    D3D12_RAYTRACING_GEOMETRY_DESC geometry_desc_;

    // Buffer for AABBs used for BLAS construction
    ComPtr<ID3D12Resource> aabb_buffer_uploader_;
    ComPtr<ID3D12Resource> aabb_buffer_;

    // Misc
    unsigned int aabb_count_ = 0;
    bool requires_rebuild_ = false;

    DX::DeviceResources* device_resources_;
};

// What needs to happen:
// - cell count used in computer.cpp to reallocate aabb buffer if needed (if number of cells has increased. could also reduce if enough cells not needed any more?)
// - compute shader then fills the aabb buffer with new aabb values
// - then, if aabb buffer size changed, BLAS is rebuilt, otherwise just updated. could also check if even needs updated (would need more compute and readback work though)
// - 