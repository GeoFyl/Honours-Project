#ifndef COMPUTE_POS_HLSL
#define COMPUTE_POS_HLSL

#include "ComputeCommon.hlsli"

RWStructuredBuffer<ParticlePosition> particle_positions_ : register(u0);
ConstantBuffer<ComputeCB> constant_buffer_ : register(b0);

// Shader for manipulating particle positions
[numthreads(1024, 1, 1)]
void CSPosMain(int3 dispatch_ID : SV_DispatchThreadID)
{
    if (dispatch_ID.x >= NUM_PARTICLES)
    {
        //ID overshoots end of buffer - nothing to do here.
        return;
    }
    
    particle_positions_[dispatch_ID.x].position_.y = particle_positions_[dispatch_ID.x].start_y_ + (0.2 * sin(0.5 * constant_buffer_.time_ * particle_positions_[dispatch_ID.x].speed_));
    
    return;
}

#endif