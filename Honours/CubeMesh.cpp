#include "stdafx.h"
#include "CubeMesh.h"
#include "Utilities.h"

CubeMesh::CubeMesh(ID3D12Device* device, ID3D12GraphicsCommandList* command_list)
{
    // Initialise vertex data
    Vertex vertices[8] =
    {
        { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(1.f, 1.f, 1.f, 1.f) },
        { XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(1.f, 0.f, 0.f, 1.f) },
        { XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(0.f, 1.f, 0.f, 1.f) },
        { XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(0.f, 0.f, 1.f, 1.f) },
        { XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(1.f, 1.f, 1.f, 1.f) },
        { XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(1.f, 0.f, 0.f, 1.f) },
        { XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(0.f, 1.f, 0.f, 1.f) },
        { XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(0.f, 0.f, 1.f, 1.f) }  
    };

    // Create vertex buffer
    UINT64 byte_size = 8 * sizeof(Vertex);
    vertex_buffer_ = Utilities::CreateDefaultBuffer(device, command_list, vertices, byte_size, vertex_buffer_uploader_);

    // Create vertex buffer view
    vertex_buffer_view_.BufferLocation = vertex_buffer_->GetGPUVirtualAddress();
    vertex_buffer_view_.StrideInBytes = sizeof(Vertex);
    vertex_buffer_view_.SizeInBytes = 8 * sizeof(Vertex);

    // Initialise index data
    UINT16 indices[36] = {
        // front face
        0, 1, 2,
        0, 2, 3,

        // back face
        4, 6, 5,
        4, 7, 6,

        // left face
        4, 5, 1,
        4, 1, 0,

        // right face
        3, 2, 6,
        3, 6, 7,

        // top face
        1, 5, 6,
        1, 6, 2,

        // bottom face
        4, 0, 3,
        4, 3, 7
    };

    // Create index buffer
    byte_size = 36 * sizeof(UINT16);
    index_buffer_ = Utilities::CreateDefaultBuffer(device, command_list, indices, byte_size, index_buffer_uploader_);

    // Create index buffer view
    index_buffer_view_.BufferLocation = index_buffer_->GetGPUVirtualAddress();
    index_buffer_view_.Format = DXGI_FORMAT_R16_UINT;
    index_buffer_view_.SizeInBytes = byte_size;

    topology_ = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
}

CubeMesh::~CubeMesh()
{
	Mesh::~Mesh();
}

void CubeMesh::Draw(ID3D12GraphicsCommandList* command_list)
{
    command_list->DrawIndexedInstanced(36, 1, 0, 0, 0);
}
