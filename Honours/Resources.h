#pragma once
#include "UploadBuffer.h"

// Number of triangle geometry objects in the scene
#define NUMBER_OF_OBJECTS 1

using namespace DirectX;
using Microsoft::WRL::ComPtr;

struct WorldViewProjectionBuffer {
	XMMATRIX world_view_proj_;
};

class Resources
{
public:
	Resources(ID3D12Device* device);
	ID3D12RootSignature* GetRootSignature() { return root_signature_.Get(); }

	void SetWorldViewProj(XMMATRIX& world_view_proj, int object_index = 0);
	void SetRootViews(ID3D12GraphicsCommandList* command_list, int object_index = 0);
private:
	void CreateConstantBuffers(ID3D12Device* device);
	void CreateRootSignatures(ID3D12Device* device);                                        

	//ComPtr<ID3D12DescriptorHeap> cb_heap_;
	ComPtr<ID3D12DescriptorHeap> srv_heap_;
	std::unique_ptr<UploadBuffer<WorldViewProjectionBuffer>> world_view_proj_cb_ = nullptr;

	// just one graphics signature rn, prob need more in future
	ComPtr<ID3D12RootSignature> root_signature_;
};

