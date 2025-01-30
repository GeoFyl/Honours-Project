#ifndef COMPUTE_GRID
#define COMPUTE_GRID

#include "ComputeCommon.hlsli"

#define CELL_MAX_PARTICLE_COUNT 8 // Also in ComputeStructs.h
#define NUM_CELLS 4096 // Also in ComputeStructs.h
#define NUM_BLOCKS 64 // Also in ComputeStructs.h

// ---- Two-level Grid -----
struct Cell
{
    int particle_count_;
    int particle_indices_[CELL_MAX_PARTICLE_COUNT];
};

struct Block
{
    int non_empty_cell_count_;
};

RWStructuredBuffer<ParticlePosition> particle_positions_ : register(u0);
RWStructuredBuffer<Cell> cells_ : register(u1);
RWStructuredBuffer<Block> blocks_ : register(u2);

// Based on http://www.gamedev.net/forums/topic/582945-find-grid-index-based-on-position/4709749/
int GetCellIndex(float3 particle_pos)
{
    // 16 x 16 x 16 cells
    float3 cell_size = WORLD_MAX / 16.f;
    
    int cell_ID = ((int) particle_pos.x / cell_size.x) + (((int) particle_pos.y / cell_size.y) * 16) + (((int) particle_pos.z / cell_size.z) * 256);
    
    return cell_ID;
}

// Shader for building the grid
[numthreads(1024, 1, 1)]
void CSPosMain(int3 dispatch_ID : SV_DispatchThreadID)
{
    if (dispatch_ID.x >= NUM_PARTICLES)
    {
        //ID overshoots end of buffer - nothing to do here.
        return;
    }
    
    float3 particle_pos = particle_positions_[dispatch_ID.x].position_;
    int cell_index = GetCellIndex(particle_pos);
    
    return;
}

#endif