#pragma once
#include <comdef.h>
#include <d3d12.h>
#include "DeviceResources.h"

using Microsoft::WRL::ComPtr;

class AccelerationStructureManager
{
public:
    AccelerationStructureManager(DX::DeviceResources* device_resources);
    void AllocateAABBBuffer(int new_aabb_count);
    ComPtr<ID3D12Resource>* GetAABBBuffer() { return &aabb_buffer_; }

private:

    // Acceleration buffers
    ComPtr<ID3D12Resource> m_accelerationStructure;
    ComPtr<ID3D12Resource> m_bottomLevelAccelerationStructure;
    ComPtr<ID3D12Resource> m_topLevelAccelerationStructure;

    // Buffer for AABBs used for BLAS construction
    ComPtr<ID3D12Resource> aabb_buffer_uploader_;
    ComPtr<ID3D12Resource> aabb_buffer_;

    unsigned int aabb_count_ = 0;

    DX::DeviceResources* device_resources_;
};

// What needs to happen:
// - cell count used in computer.cpp to reallocate aabb buffer if needed (if number of cells has increased. could also reduce if enough cells not needed any more?)
// - compute shader then fills the aabb buffer with new aabb values
// - then, if aabb buffer size changed, BLAS is rebuilt, otherwise just updated. could also check if even needs updated (would need more compute and readback work though)
// - 