#pragma once
#include <DirectXMath.h>

#define MAX_RECURSION_DEPTH 1 // Primary rays

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
	XMFLOAT3 camera_pos_;
};