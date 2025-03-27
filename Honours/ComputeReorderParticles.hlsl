#ifndef COMPUTE_REORDER_HLSL
#define COMPUTE_REORDER_HLSL

#include "ComputeCommon.hlsli"

RWStructuredBuffer<ParticleData> particles_ordered_ : register(u0);
StructuredBuffer<ParticleData> particles_unordered_ : register(t0);
StructuredBuffer<uint> cell_global_index_offsets_ : register(t1);

// Shader for manipulating particle positions
[numthreads(1024, 1, 1)]
void CSReorderParticlesMain(int3 dispatch_ID : SV_DispatchThreadID)
{
    if (dispatch_ID.x >= NUM_PARTICLES)
    {
        //ID overshoots end of buffer - nothing to do here.
        return;
    }
    
    ParticleData particle_data = particles_unordered_[dispatch_ID.x];
    
    // Calculate new index from global and intra-cell offsets. Copy the data to the new address in the sorted buffer.
    uint new_index = cell_global_index_offsets_[particle_data.cell_index_] + particle_data.intra_cell_index_;
    particles_ordered_[new_index] = particle_data;
    
    return;
}

#endif