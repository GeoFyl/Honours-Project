#pragma once
#include <initguid.h>
#include <d3d12.h>
#include <comdef.h>

class Utilities {
public :
	// Method for creating a default buffer as described in (Luna, 2016)
	static Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultBuffer(ID3D12Device* device, ID3D12GraphicsCommandList* command_list, const void* data, UINT64 byte_size, Microsoft::WRL::ComPtr<ID3D12Resource>& upload_buffer);
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
	HRESULT hr = (x); \
	WCHAR buffer[512]; \
	MultiByteToWideChar(CP_ACP, 0, std::string(__FILE__).c_str(), -1, buffer, 512); \
	if(FAILED(hr)) {throw DxException(hr, L#x, std::wstring(buffer), __LINE__);} \
}
#endif

