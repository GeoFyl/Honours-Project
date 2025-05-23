#ifndef COMPUTE_AABBS
#define COMPUTE_AABBS

#include "ComputeGridCommon.hlsli"

StructuredBuffer<uint> surface_cell_indices_ : register(t0);
StructuredBuffer<GridSurfaceCounts> surface_counts_ : register(t1);
RWStructuredBuffer<AABB> aabbs_ : register(u0);

// Builds Array of AABBs in for surface cells world space
[numthreads(1024, 1, 1)]
void CSBuildAABBs(int3 dispatch_ID : SV_DispatchThreadID)
{
    int cells_count = surface_counts_[0].surface_cells;
    if (dispatch_ID.x >= cells_count * BRICKS_PER_CELL)
    {
        //Not enough cells
        return;
    }
    
    // Determine which cell we're creating an AABB within
    uint surface_cell_array_index = dispatch_ID.x / BRICKS_PER_CELL;
    uint grid_cell_index = surface_cell_indices_[surface_cell_array_index];
    
    // Convert from grid-space cell coords to world-space
    float3 grid_cell_coords_3d = CellIndexTo3DCoords(grid_cell_index);    
    float3 world_pos = lerp(WORLD_MIN, WORLD_MAX, grid_cell_coords_3d / float3(NUM_CELLS_PER_AXIS));
    
    // Determine which brick within the cell the AABB is for
    uint intra_cell_brick_index = dispatch_ID.x % BRICKS_PER_CELL;    
    float3 brick_offset = BrickIndexTo3DOffset(intra_cell_brick_index);
    
    // Create and store the AABB
    AABB aabb;
    aabb.min_ = world_pos + (brick_offset * BRICK_SIZE);
    aabb.max_ = aabb.min_ + BRICK_SIZE;

    aabbs_[dispatch_ID.x] = aabb;
}

#endif