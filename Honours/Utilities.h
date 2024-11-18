#pragma once
#include <initguid.h>
#include <d3d12.h>
#include <comdef.h>

class Utilities {
public :
	// Method for creating a default buffer as described in (Luna, 2016)
	static Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultBuffer(ID3D12Device* device, ID3D12GraphicsCommandList* command_list, const void* data, UINT64 byte_size, Microsoft::WRL::ComPtr<ID3D12Resource>& upload_buffer);
	static void AllocateDefaultBuffer(ID3D12Device* device, UINT64 buffer_size, ID3D12Resource** buffer, D3D12_RESOURCE_STATES initial_state = D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

	static inline UINT Align(UINT size, UINT alignment) {
		return (size + (alignment - 1)) & ~(alignment - 1);
	}
};

// Class used in handling HRESULT errors as described in (Luna, 2016)
class DxException {
public:
	DxException() = default;
	DxException(HRESULT hr, const std::wstring& function_name, const std::wstring& filename, int line_number);
	std::wstring ToString() const;

	HRESULT error_ = S_OK;
	std::wstring function_;
	std::wstring file_;
	int line_number_ = -1;
};

#ifndef ThrowIfFailed
#define ThrowIfFailed(x) { \
	HRESULT res = (x); \
	WCHAR buffer[512]; \
	MultiByteToWideChar(CP_ACP, 0, std::string(__FILE__).c_str(), -1, buffer, 512); \
	if(FAILED(res)) {throw DxException(res, L#x, std::wstring(buffer), __LINE__);} \
}
#endif

