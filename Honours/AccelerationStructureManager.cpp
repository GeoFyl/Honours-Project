#include "stdafx.h"
#include "AccelerationStructureManager.h"
#include "Utilities.h"

AccelerationStructureManager::AccelerationStructureManager(DX::DeviceResources* device_resources) :
	device_resources_(device_resources)
{
}

void AccelerationStructureManager::AllocateAABBBuffer(int new_aabb_count)
{
	// Release and reallocate buffer for AABBs if required number has increased
	if (new_aabb_count > aabb_count_) {
		ID3D12Device5* device = device_resources_->GetD3DDevice();
		ID3D12GraphicsCommandList4* command_list = device_resources_->GetCommandList();

		aabb_count_ = new_aabb_count;

		aabb_buffer_.Reset();
		Utilities::AllocateDefaultBuffer(device, aabb_count_ * sizeof(D3D12_RAYTRACING_AABB), &aabb_buffer_, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	}
}
