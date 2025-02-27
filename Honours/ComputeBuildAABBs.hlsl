#ifndef COMPUTE_AABBS
#define COMPUTE_AABBS

#include "ComputeCommon.hlsli"
#include "ComputeGridCommon.hlsli"

//RWStructuredBuffer<Cell> cells_ : register(u1);
//RWStructuredBuffer<Block> blocks_ : register(u2);
//RWStructuredBuffer<uint> surface_block_indices_ : register(u3);
StructuredBuffer<uint> surface_cell_indices_ : register(t0);
StructuredBuffer<GridSurfaceCounts> surface_counts_ : register(t1);
RWStructuredBuffer<AABB> aabbs_ : register(u0);


// Blocks: 4 x 4 x 4 total
// Cells: 16 x 16 x 16 total, 4 x 4 x 4 per block

// Clears the count of surface blocks and cells
[numthreads(1024, 1, 1)]
void CSBuildAABBs(int3 dispatch_ID : SV_DispatchThreadID)
{
    if (dispatch_ID.x >= surface_counts_[0].surface_cells)
    {
        //Not enough cells
        return;
    }
    
    uint cell_index = surface_cell_indices_[dispatch_ID.x];


}

#endif