#include "stdafx.h"
#include "Mesh.h"

Mesh::~Mesh()
{
	vertex_buffer_.Reset();
	vertex_buffer_uploader_.Reset();
	index_buffer_.Reset();
	index_buffer_uploader_.Reset();
}

void Mesh::SendData(ID3D12GraphicsCommandList* command_list)
{
	command_list->IASetPrimitiveTopology(topology_);
	command_list->IASetVertexBuffers(0, 1, &vertex_buffer_view_);
	if (index_buffer_) command_list->IASetIndexBuffer(&index_buffer_view_);
}

void Mesh::ReleaseUploaders()
{
	vertex_buffer_uploader_.Reset();
	index_buffer_uploader_.Reset();
}
