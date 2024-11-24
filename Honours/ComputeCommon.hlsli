#ifndef COMPUTE_COMMON_HLSL
#define COMPUTE_COMMON_HLSL

#define NUM_PARTICLES 50
#define TEXTURE_RESOLUTION 256

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