#ifndef COMPUTE_COMMON_HLSL
#define COMPUTE_COMMON_HLSL

#define NUM_PARTICLES 125 // Also in ComputeStructs.h, RayTracingCommon.hlsli
#define TEXTURE_RESOLUTION 256 // Also in ComputeStructs.h, RayTracingCommon.hlsli

#define WORLD_MIN float3(0,0,0)
#define WORLD_MAX float3(2,2,2)

struct ParticlePosition
{
    float3 position_;
    float speed_;
    float start_y_;
};

struct ComputeCB
{
    float time_;
};

struct AABB
{
    float3 min_;
    float3 max_;
};



#endif