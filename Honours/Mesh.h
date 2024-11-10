#pragma once
#include <d3d12.h>

using namespace DirectX;

class Mesh
{
public:
	~Mesh();

	void SendData(ID3D12GraphicsCommandList* command_list);
	void ReleaseUploaders();
	virtual void Draw(ID3D12GraphicsCommandList* command_list) = 0;
protected:
	struct Vertex {
		XMFLOAT3 pos_;
		XMFLOAT4 colour_;
	};

	Microsoft::WRL::ComPtr<ID3D12Resource> vertex_buffer_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> vertex_buffer_uploader_ = nullptr;
	D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view_;

	Microsoft::WRL::ComPtr<ID3D12Resource> index_buffer_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> index_buffer_uploader_ = nullptr;
	D3D12_INDEX_BUFFER_VIEW index_buffer_view_;

	D3D_PRIMITIVE_TOPOLOGY topology_;
};

