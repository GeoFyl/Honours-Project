#ifndef COMPUTE_COMMON_HLSL
#define COMPUTE_COMMON_HLSL

#define NUM_PARTICLES 125 // Also in ComputeStructs.h, RayTracingCommon.hlsli
#define TEXTURE_RESOLUTION 256 // Also in ComputeStructs.h

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

#endif