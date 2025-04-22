#pragma once
#include <DirectXMath.h>
#include "TestValues.h"

#define MAX_RECURSION_DEPTH 1 // Primary rays

// Rendering flags
#define RENDERING_FLAG_NONE                     0
#define RENDERING_FLAG_VISUALIZE_PARTICLES      1 << 0
#define RENDERING_FLAG_ANALYTICAL               1 << 1
#define RENDERING_FLAG_NORMALS                  1 << 2
#define RENDERING_FLAG_VISUALIZE_AABBS          1 << 3
#define RENDERING_FLAG_SIMPLE_AABB              1 << 4

using namespace DirectX;

struct RayPayload
{
    XMFLOAT4 colour_;
};

struct RayIntersectionAttributes
{
    XMFLOAT3 float_3_; // Can be used for normals or brick AABB uvw 
};

struct RayTracingCB {
    XMMATRIX view_proj_;
    XMMATRIX inv_view_proj_;
	XMFLOAT3 camera_pos_;
    float padding_;
	XMFLOAT3 camera_lookat_;
    UINT rendering_flags_ = 0;
};

struct AABB
{
    XMFLOAT3 min_;
    XMFLOAT3 max_;
};