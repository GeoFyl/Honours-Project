#pragma once
#include <DirectXMath.h>

#define NUM_PARTICLES 50
#define TEXTURE_RESOLUTION 256

using namespace DirectX;

struct ParticlePosition {
	XMFLOAT3 position_;
	float speed_;
	float start_y_;
};

struct ComputeCB {
	float time_ = 0;
};