#ifndef COMPUTE_COMMON_HLSL
#define COMPUTE_COMMON_HLSL

#define NUM_PARTICLES 50

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

RWStructuredBuffer<ParticlePosition> particle_positions_ : register(u0);
ConstantBuffer<ComputeCB> constant_buffer_ : register(b0);

#endif