#ifndef COMPUTE_COMMON_HLSL
#define COMPUTE_COMMON_HLSL

#include "GlobalValues.hlsli"

struct ParticlePosition
{
    float3 position_;
    float speed_;
    float start_y_;
};

struct ComputeCB
{
    float time_;
    uint3 brick_pool_dimensions_;
};

struct AABB
{
    float3 min_;
    float3 max_;
};



#endif