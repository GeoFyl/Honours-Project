#include "stdafx.h"
#include "TriangleMesh.h"
#include "Utilities.h"

TriangleMesh::TriangleMesh(ID3D12Device* device, ID3D12GraphicsCommandList* command_list)
{
    // Initialise vertex data
    Vertex vertices[3] =
    {
        { XMFLOAT3(0.0f, 0.25f, 0.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) },
        { XMFLOAT3(0.25f, -0.25f, 0.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) },
        { XMFLOAT3(-0.25f, -0.25f, 0.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) }
    };

    // Create vertex buffer
    const UINT64 byte_size = 3 * sizeof(Vertex);
    vertex_buffer_ = Utilities::CreateDefaultBuffer(device, command_list, vertices, byte_size, vertex_buffer_uploader_);

    // Create vertex buffer view
    vertex_buffer_view_.BufferLocation = vertex_buffer_->GetGPUVirtualAddress();
    vertex_buffer_view_.StrideInBytes = sizeof(Vertex);
    vertex_buffer_view_.SizeInBytes = 3 * sizeof(Vertex);

    topology_ = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
}

TriangleMesh::~TriangleMesh()
{
    Mesh::~Mesh();
}

void TriangleMesh::Draw(ID3D12GraphicsCommandList* command_list)
{
    command_list->DrawInstanced(3, 1, 0, 0);
}
