#ifndef COMPUTE_POS_HLSL
#define COMPUTE_POS_HLSL

#include "ComputeCommon.hlsli"

RWStructuredBuffer<ParticleData> particle_positions_ : register(u0);
ConstantBuffer<ComputeCB> constant_buffer_ : register(b1);

// Shader for manipulating particle positions
[numthreads(1024, 1, 1)]
void CSPosMain(int3 dispatch_ID : SV_DispatchThreadID)
{
    if (dispatch_ID.x >= NUM_PARTICLES)
    {
        //ID overshoots end of buffer - nothing to do here.
        return;
    }
    
    float new_y = particle_positions_[dispatch_ID.x].start_y_ + (0.2 * sin(0.5 * constant_buffer_.time_ * particle_positions_[dispatch_ID.x].speed_));
    
    if (new_y > WORLD_MAX.y)
    {
        new_y = WORLD_MAX.y - 0.1f;
    }
    else if (new_y < 0)
    {
        new_y = 0.1f;
    }
    
    particle_positions_[dispatch_ID.x].position_.y = new_y;
    
    return;
}

#endif