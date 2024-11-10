#pragma once
#include "Mesh.h"
class CubeMesh : public Mesh
{
public:
	CubeMesh(ID3D12Device* device, ID3D12GraphicsCommandList* command_list);
	~CubeMesh();

	void Draw(ID3D12GraphicsCommandList* command_list) override;
};

