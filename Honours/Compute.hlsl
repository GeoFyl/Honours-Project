#ifndef COMPUTE_HLSL
#define COMPUTE_HLSL

#include "ComputeCommon.hlsli"

ConsumeStructuredBuffer<ParticlePosition> particle_pos_input_ : register(u0);
AppendStructuredBuffer<ParticlePosition> particle_pos_output_ : register(u1);

[numthreads(32, 1, 1)]
void CSMain(int3 dispatch_ID : SV_DispatchThreadID)
{
    ParticlePosition pos = particle_pos_input_.Consume();
    if (pos.position_.x == 0 && pos.position_.y == 0 && pos.position_.z == 0 && pos.speed == 0)
    {
        // Consumed past end of buffer, nothing to do
        return;
    }
    particle_pos_output_.Append(pos);
    return;
}

#endif