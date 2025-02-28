#ifndef COMPUTE_AABBS
#define COMPUTE_AABBS

#include "ComputeGridCommon.hlsli"

//RWStructuredBuffer<Cell> cells_ : register(u1);
//RWStructuredBuffer<Block> blocks_ : register(u2);
//RWStructuredBuffer<uint> surface_block_indices_ : register(u3);
StructuredBuffer<uint> surface_cell_indices_ : register(t0);
StructuredBuffer<GridSurfaceCounts> surface_counts_ : register(t1);
RWStructuredBuffer<AABB> aabbs_ : register(u0);


// Blocks: 4 x 4 x 4 total
// Cells: 16 x 16 x 16 total, 4 x 4 x 4 per block

// Builds Array of AABBs in for surface cells world space
[numthreads(1024, 1, 1)]
void CSBuildAABBs(int3 dispatch_ID : SV_DispatchThreadID)
{
    if (dispatch_ID.x >= surface_counts_[0].surface_cells)
    {
        //Not enough cells
        return;
    }
    
    uint cell_index = surface_cell_indices_[dispatch_ID.x];
    uint3 coords_3d = CellIndexTo3DCoords(cell_index);
    
    // Convert from grid-space cell coords to world-space
    float3 world_pos = lerp(WORLD_MIN, WORLD_MAX, coords_3d / float3(16.f, 16.f, 16.f));
    
    float3 half_cell_size = WORLD_MAX / 32.f;  // 32 since (1/16)/2 = 1/32 
    
    AABB aabb;
    aabb.min_ = world_pos;
    aabb.max_ = world_pos + half_cell_size;

    aabbs_[dispatch_ID.x] = aabb;
}

#endif