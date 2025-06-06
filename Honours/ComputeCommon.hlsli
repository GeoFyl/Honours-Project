#ifndef COMPUTE_COMMON_HLSL
#define COMPUTE_COMMON_HLSL

#include "GlobalValues.hlsli"

struct ParticleData
{
    float3 position_;
    float3 start_pos_;    
    float speed_;
    uint intra_cell_index_;
    uint cell_index_;
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