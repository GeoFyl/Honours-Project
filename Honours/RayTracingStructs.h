#pragma once
#include <DirectXMath.h>

#define MAX_RECURSION_DEPTH 1 // Primary rays

// Rendering flags
#define RENDERING_FLAG_NONE 0
#define RENDERING_FLAG_VISUALIZE_PARTICLES 1

using namespace DirectX;

struct RayPayload
{
    XMFLOAT4 colour_;
};

struct RayIntersectionAttributes
{
    float example;
};

struct RayTracingCB {
    XMMATRIX view_proj_;
    XMMATRIX inv_view_proj_;
	XMFLOAT3 camera_pos_;
    UINT rendering_flags_;
};