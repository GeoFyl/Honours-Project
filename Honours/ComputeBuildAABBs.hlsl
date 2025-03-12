#ifndef COMPUTE_AABBS
#define COMPUTE_AABBS

#include "ComputeGridCommon.hlsli"

//RWStructuredBuffer<Cell> cells_ : register(u1);
//RWStructuredBuffer<Block> blocks_ : register(u2);
//RWStructuredBuffer<uint> surface_block_indices_ : register(u3);
StructuredBuffer<uint> surface_cell_indices_ : register(t0);
StructuredBuffer<GridSurfaceCounts> surface_counts_ : register(t1);
RWStructuredBuffer<AABB> aabbs_ : register(u0);

// Takes the index of a brick within a cell to determine an offset
uint3 BrickIndexTo3DOffset(uint brick_index)
{
    uint3 offset = uint3(0, 0, 0);
    
    uint bricks_per_z = BRICKS_PER_AXIS_PER_CELL * BRICKS_PER_AXIS_PER_CELL;
    offset.z = brick_index / bricks_per_z;
    offset.y = (brick_index % bricks_per_z) / BRICKS_PER_AXIS_PER_CELL;
    offset.x = brick_index % BRICKS_PER_AXIS_PER_CELL;
    
    return offset;
}

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
    
    uint surface_cell_array_index = dispatch_ID.x / BRICKS_PER_CELL;
    uint intra_cell_brick_index = dispatch_ID.x % BRICKS_PER_CELL;
    
    uint grid_cell_index = surface_cell_indices_[surface_cell_array_index];
    float3 grid_cell_coords_3d = CellIndexTo3DCoords(grid_cell_index);
    
    // Convert from grid-space cell coords to world-space
    float3 world_pos = lerp(WORLD_MIN, WORLD_MAX, grid_cell_coords_3d / float3(NUM_CELLS_PER_AXIS));
    
    float3 brick_offset = BrickIndexTo3DOffset(intra_cell_brick_index);
    
    AABB aabb;
    aabb.min_ = world_pos + (brick_offset * BRICK_SIZE);
    aabb.max_ = aabb.min_ + BRICK_SIZE;

    aabbs_[dispatch_ID.x] = aabb;
    
    //int cells_count = surface_counts_[0].surface_cells;
    //if (dispatch_ID.x >= cells_count)
    //{
    //    //Not enough cells
    //    return;
    //}
    
    //uint cell_index = surface_cell_indices_[dispatch_ID.x];
    //float3 coords_3d = CellIndexTo3DCoords(cell_index);
    
    //// Convert from grid-space cell coords to world-space
    //float3 world_pos = lerp(WORLD_MIN, WORLD_MAX, coords_3d / float3(NUM_CELLS_PER_AXIS));
    
    //AABB aabb;
    //aabb.min_ = world_pos;
    //aabb.max_ = world_pos + CELL_SIZE;

    //aabbs_[dispatch_ID.x] = aabb;
}

#endif