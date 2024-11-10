#include "stdafx.h"
#include "Resources.h"

Resources::Resources(ID3D12Device* device)
{
	CreateConstantBuffers(device);
	CreateRootSignatures(device);
}

void Resources::SetWorldViewProj(XMMATRIX& world_view_proj, int object_index)
{
	WorldViewProjectionBuffer buff;
	buff.world_view_proj_ = world_view_proj;

	world_view_proj_cb_->CopyData(object_index, buff);
}

void Resources::SetRootViews(ID3D12GraphicsCommandList* command_list, int object_index)
{
	// Set the root constant buffer view for the world-view-projection matrix
	UINT address_offset = world_view_proj_cb_->GetCBByteSize() * object_index;
	command_list->SetGraphicsRootConstantBufferView(0, world_view_proj_cb_->Resource()->GetGPUVirtualAddress() + address_offset);
}

void Resources::CreateConstantBuffers(ID3D12Device* device)
{
	// Create constant buffer descriptor heap
	/*D3D12_DESCRIPTOR_HEAP_DESC cb_heap_desc;
	cb_heap_desc.NumDescriptors = 1;
	cb_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cb_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cb_heap_desc.NodeMask = 0;
	ThrowIfFailed(device->CreateDescriptorHeap(&cb_heap_desc, IID_PPV_ARGS(&cb_heap_)));*/

	// Create world-view-projection matrix constant buffer
	world_view_proj_cb_ = std::make_unique<UploadBuffer<WorldViewProjectionBuffer>>(device, NUMBER_OF_OBJECTS, true);

	/*D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc;
	cbv_desc.BufferLocation = world_view_proj_cb_->Resource()->GetGPUVirtualAddress();
	cbv_desc.SizeInBytes = world_view_proj_cb_->GetCBByteSize();

	device->CreateConstantBufferView(&cbv_desc, cb_heap_->GetCPUDescriptorHandleForHeapStart());*/
}

void Resources::CreateRootSignatures(ID3D12Device* device)
{
	CD3DX12_ROOT_PARAMETER cb_descriptor;
	cb_descriptor.InitAsConstantBufferView(0);

	CD3DX12_ROOT_SIGNATURE_DESC root_sig_desc_(1, &cb_descriptor, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	ThrowIfFailed(D3D12SerializeRootSignature(&root_sig_desc_, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
	ThrowIfFailed(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&root_signature_)));
}
