#include "stdafx.h"
#include "AccelerationStructureManager.h"
#include "Utilities.h"


AccelerationStructureManager::AccelerationStructureManager(DX::DeviceResources* device_resources) :
	device_resources_(device_resources)
{
	ID3D12Device5* device = device_resources_->GetD3DDevice();

	// Get prebuild info for TLAS
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
	top_level_inputs_.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	top_level_inputs_.Flags = buildFlags;
	top_level_inputs_.NumDescs = 1;
	top_level_inputs_.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	device->GetRaytracingAccelerationStructurePrebuildInfo(&top_level_inputs_, &top_level_prebuild_info_);

	// Set constant values in geometry description
	geometry_desc_.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS;
	geometry_desc_.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE; // change later
	geometry_desc_.AABBs.AABBs.StrideInBytes = sizeof(D3D12_RAYTRACING_AABB);

	// Allocate resource for TLAS
	Utilities::AllocateDefaultBuffer(device, top_level_prebuild_info_.ResultDataMaxSizeInBytes, &top_acceleration_structure_, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	// Create an instance desc for the bottom-level acceleration structure.
	instance_desc_buffer = std::make_unique<UploadBuffer<D3D12_RAYTRACING_INSTANCE_DESC>>(device, 1, false);
}

void AccelerationStructureManager::AllocateAABBBuffer(int new_aabb_count)
{
	if (new_aabb_count != aabb_count_) {
		requires_rebuild_ = true;
	}
	else {
		requires_rebuild_ = false;
	}

	// Release and reallocate buffer for AABBs if required number has increased
	if (new_aabb_count > aabb_count_) {
		ID3D12Device5* device = device_resources_->GetD3DDevice();
		ID3D12GraphicsCommandList4* command_list = device_resources_->GetCommandList();

		aabb_count_ = new_aabb_count;

		aabb_buffer_.Reset();
		Utilities::AllocateDefaultBuffer(device, aabb_count_ * sizeof(D3D12_RAYTRACING_AABB), &aabb_buffer_, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	}
}

// Updates or rebuilds acceleration structure
void AccelerationStructureManager::UpdateStructure()
{
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO blas_prebuild_info;
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS blas_inputs;
	CalculatePreBuildInfo(blas_inputs, blas_prebuild_info);

	// Fully rebuild if needed, otherwise update with new geometry
	if (requires_rebuild_) {
		RebuildStructure(blas_inputs, blas_prebuild_info);
	}
	else {
		BuildStructures(blas_inputs, blas_prebuild_info, true);
	}

	// Execute and wait for work to finish 
	device_resources_->ExecuteCommandList();
	device_resources_->WaitForGpu();
}

void AccelerationStructureManager::CalculatePreBuildInfo(D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& blas_inputs, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO& blas_prebuild_info)
{
	// Build geometry description
	geometry_desc_.AABBs.AABBCount = aabb_count_;
	geometry_desc_.AABBs.AABBs.StartAddress = aabb_buffer_->GetGPUVirtualAddress();

	// Get prebuild info for BLAS
	blas_inputs = top_level_inputs_;
	blas_inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	blas_inputs.pGeometryDescs = &geometry_desc_;
	device_resources_->GetD3DDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&blas_inputs, &blas_prebuild_info);
}

void AccelerationStructureManager::RebuildStructure(D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& blas_inputs, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO& blas_prebuild_info)
{
	ID3D12Device5* device = device_resources_->GetD3DDevice();

	// If required BLAS size has increased, will need to reallocate
	UINT64 current_blas_width = bottom_acceleration_structure_ ? bottom_acceleration_structure_->GetDesc().Width : 0;
	if (blas_prebuild_info.ResultDataMaxSizeInBytes > current_blas_width) {
		// Release previous resource
		bottom_acceleration_structure_.Reset();

		// Allocate resource for BLAS
		Utilities::AllocateDefaultBuffer(device, blas_prebuild_info.ResultDataMaxSizeInBytes, &bottom_acceleration_structure_, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

		// Update the instance desc for the bottom-level acceleration structure.
		D3D12_RAYTRACING_INSTANCE_DESC instanceDesc = {};
		instanceDesc.Transform[0][0] = instanceDesc.Transform[1][1] = instanceDesc.Transform[2][2] = 1;
		instanceDesc.InstanceMask = 1;
		instanceDesc.AccelerationStructure = bottom_acceleration_structure_->GetGPUVirtualAddress();
		instance_desc_buffer->CopyData(0, instanceDesc);
	}
	// If required scratch size has increased, will need to reallocate
	UINT64 current_scratch_width = scratch_resource_ ? scratch_resource_->GetDesc().Width : 0;
	if (blas_prebuild_info.ScratchDataSizeInBytes > current_scratch_width) {
		// Release previous resource
		scratch_resource_.Reset();

		// Allocate resource for scratch
		Utilities::AllocateDefaultBuffer(device, max(top_level_prebuild_info_.ScratchDataSizeInBytes, blas_prebuild_info.ScratchDataSizeInBytes), &scratch_resource_, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	}

	BuildStructures(blas_inputs, blas_prebuild_info);
}

void AccelerationStructureManager::BuildStructures(D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& blas_inputs, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO& blas_prebuild_info, bool update)
{
	ID3D12Device5* device = device_resources_->GetD3DDevice();
	ID3D12GraphicsCommandList4* command_list = device_resources_->GetCommandList();

	// Bottom Level Acceleration Structure desc
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildDesc = {};
	{
		bottomLevelBuildDesc.Inputs = blas_inputs;
		bottomLevelBuildDesc.ScratchAccelerationStructureData = scratch_resource_->GetGPUVirtualAddress();
		bottomLevelBuildDesc.DestAccelerationStructureData = bottom_acceleration_structure_->GetGPUVirtualAddress();
		if (update) {
			bottomLevelBuildDesc.Inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
			bottomLevelBuildDesc.SourceAccelerationStructureData = bottom_acceleration_structure_->GetGPUVirtualAddress();
		}
	}

	// Top Level Acceleration Structure desc
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelBuildDesc = {};
	{
		top_level_inputs_.InstanceDescs = instance_desc_buffer->Resource()->GetGPUVirtualAddress();
		topLevelBuildDesc.Inputs = top_level_inputs_;
		topLevelBuildDesc.Inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
		topLevelBuildDesc.SourceAccelerationStructureData = top_acceleration_structure_->GetGPUVirtualAddress();
		topLevelBuildDesc.DestAccelerationStructureData = top_acceleration_structure_->GetGPUVirtualAddress();
		topLevelBuildDesc.ScratchAccelerationStructureData = scratch_resource_->GetGPUVirtualAddress();
	}

	device_resources_->ResetCommandList();
	command_list->BuildRaytracingAccelerationStructure(&bottomLevelBuildDesc, 0, nullptr);
	command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(bottom_acceleration_structure_.Get()));
	command_list->BuildRaytracingAccelerationStructure(&topLevelBuildDesc, 0, nullptr);
}
