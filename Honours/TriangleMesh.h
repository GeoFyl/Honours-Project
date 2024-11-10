#pragma once
#include "Mesh.h"

class TriangleMesh :  public Mesh
{
public:
	TriangleMesh(ID3D12Device* device, ID3D12GraphicsCommandList* command_list);
	~TriangleMesh();

	void Draw(ID3D12GraphicsCommandList* command_list) override;
};

