#pragma once
#include <DirectXMath.h>

#define NUM_PARTICLES 125 // Also in ComputeCommon.hlsli, RayTracingCommon.hlsli
#define TEXTURE_RESOLUTION 256 // Also in ComputeCommon.hlsli

using namespace DirectX;

struct ParticlePosition {
	XMFLOAT3 position_;
	float speed_;
	float start_y_;
};

struct ComputeCB {
	float time_ = 0;
};