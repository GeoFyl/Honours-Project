#include "stdafx.h"
#include "Utilities.h"

DxException::DxException(HRESULT hr, const std::wstring& function_name, const std::wstring& filename, int line_number) :
	error_(hr), function_(function_name), file_(filename), line_number_(line_number)
{
}

std::wstring DxException::ToString() const
{
	return function_ + L" failed in " + file_ + L"; line " + std::to_wstring(line_number_) + L"; error: " + _com_error(error_).ErrorMessage();
}

using namespace Microsoft::WRL;
ComPtr<ID3D12Resource> Utilities::CreateDefaultBuffer(ID3D12Device* device, ID3D12GraphicsCommandList* command_list, const void* data, UINT64 byte_size, ComPtr<ID3D12Resource>& upload_buffer, D3D12_RESOURCE_STATES initial_state, D3D12_RESOURCE_FLAGS flags)
{
	// Create default buffer resource
	ComPtr<ID3D12Resource> default_buffer;
	AllocateDefaultBuffer(device, byte_size, default_buffer.GetAddressOf(), initial_state, flags);

	// Create intermediate upload resource
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(byte_size),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(upload_buffer.GetAddressOf())));

	// Describe data being copied
	D3D12_SUBRESOURCE_DATA data_desc = {};
	data_desc.pData = data;
	data_desc.RowPitch = byte_size;
	data_desc.SlicePitch = data_desc.RowPitch;

	// Schedule copy - must keep upload buffer alive till copy has been executed!
	command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(default_buffer.Get(), initial_state, D3D12_RESOURCE_STATE_COPY_DEST));

	UpdateSubresources<1>(command_list, default_buffer.Get(), upload_buffer.Get(), 0, 0, 1, &data_desc);

	command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(default_buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, initial_state));

	return default_buffer;
}

void Utilities::AllocateDefaultBuffer(ID3D12Device* device, UINT64 buffer_size, ID3D12Resource** buffer, D3D12_RESOURCE_STATES initial_state, D3D12_RESOURCE_FLAGS flags)
{
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(buffer_size, flags),
		initial_state,
		nullptr,
		IID_PPV_ARGS(buffer)));
}

