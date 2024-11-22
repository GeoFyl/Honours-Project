#ifndef COMPUTE_HLSL
#define COMPUTE_HLSL

#include "ComputeCommon.hlsli"

[numthreads(256, 1, 1)]
void CSMain(int3 dispatch_ID : SV_DispatchThreadID)
{
    if (dispatch_ID.x >= NUM_PARTICLES)
    {
        //ID overshoots end of buffer - nothing to do here.
        return;
    }
    
    particle_positions_[dispatch_ID.x].position_.y = particle_positions_[dispatch_ID.x].start_y_ + (0.2 * sin(0.5 * constant_buffer_.time_ * particle_positions_[dispatch_ID.x].speed_));
    //particle_positions_[dispatch_ID.x].position_.y = 0.f;
    
    return;
}

#endif