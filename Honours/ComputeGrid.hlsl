#ifndef COMPUTE_GRID
#define COMPUTE_GRID

#include "ComputeGridCommon.hlsli"

StructuredBuffer<ParticlePosition> particle_positions_ : register(t0);
RWStructuredBuffer<Cell> cells_ : register(u1);
RWStructuredBuffer<Block> blocks_ : register(u2);
RWStructuredBuffer<uint> surface_block_indices_ : register(u3);
RWStructuredBuffer<uint> surface_cell_indices_ : register(u4);
RWStructuredBuffer<GridSurfaceCounts> surface_counts_ : register(u5);


// Blocks: 4 x 4 x 4 total
// Cells: 16 x 16 x 16 total, 4 x 4 x 4 per block

// Finds if a neighbouring cell is empty, given a current index and an offset
// Returns true if neighbour cell exists
bool IsNeighbourEmpty(int cell_index, int3 offset, out bool empty)
{
    uint3 cell_coords = CellIndexTo3DCoords(cell_index) + offset;
    
    uint3 cells_per_axis = uint3(NUM_CELLS_PER_AXIS);
    int new_index = (cell_coords.z * cells_per_axis.x * cells_per_axis.y) + (cell_coords.y * cells_per_axis.x) + cell_coords.x;
    
    if (new_index > -1 && new_index < NUM_CELLS)
    {
        empty = (cells_[new_index].particle_count_ == 0);   
        return true;
    }

    return false;
}

void MarkAsSurfaceCell(uint cell_index)
{
    uint surface_cells_array_index;
    InterlockedAdd(surface_counts_[0].surface_cells, 1, surface_cells_array_index);
    surface_cell_indices_[surface_cells_array_index] = cell_index;
    return;
}

// --------------- SHADERS -------------------

// Clears the count of surface blocks and cells
[numthreads(1024, 1, 1)]
void CSClearGridCounts(int3 dispatch_ID : SV_DispatchThreadID)
{
    if (dispatch_ID.x == 0)
    {
        surface_counts_[0].surface_blocks = 0;
        surface_counts_[0].surface_cells = 0;        
    }
    if (dispatch_ID.x <= NUM_CELLS)
    {
        cells_[dispatch_ID.x].particle_count_ = 0;
    }
    if (dispatch_ID.x <= NUM_BLOCKS)
    {
        blocks_[dispatch_ID.x].non_empty_cell_count_ = 0;
    }
}


// Shader for building the grid
[numthreads(1024, 1, 1)]
void CSGridMain(int3 dispatch_ID : SV_DispatchThreadID)
{
    if (dispatch_ID.x >= NUM_PARTICLES)
    {
        //ID overshoots end of buffer - nothing to do here.
        return;
    }
    
    // Identify the cell containing the target particle
    float3 particle_pos = particle_positions_[dispatch_ID.x].position_;
    uint cell_index = GetCellIndex(particle_pos);
    
    // Increment the cell's particle count and assign intra-cell offset
    uint particle_intra_cell_index;
    InterlockedAdd(cells_[cell_index].particle_count_, 1, particle_intra_cell_index);
    
    // Add the particle's index to cell's particle list
    if (particle_intra_cell_index < CELL_MAX_PARTICLE_COUNT)
    {
        cells_[cell_index].particle_indices_[particle_intra_cell_index] = dispatch_ID.x;        
    }
    
    // If this is the first particle in the cell, increment the blocks non empty cell counter
    if (particle_intra_cell_index == 0)
    {
        int block_indices[8];
        CellIndexToBlockIndices(cell_index, block_indices);
        
        //uint block_index = CellIndexToBlockIndex(cell_index);
        
        for (int i = 0; i < 8; i++)
        {
            if (block_indices[i]> -1)
                InterlockedAdd(blocks_[block_indices[i]].non_empty_cell_count_, 1);
        } 
        
    }
    
    return;
}

// Shader for detecting surface blocks
[numthreads(1024, 1, 1)]
void CSDetectSurfaceBlocksMain(int3 dispatch_ID : SV_DispatchThreadID)
{
    
    if (dispatch_ID.x >= NUM_BLOCKS)
    {
        //ID overshoots end of buffer - nothing to do here.
        return;
    }
    
    uint block_index = dispatch_ID.x;
    
    // If the non-empty cell count is 0, or 64 and not at the edge, the block is not a surface block
    // Technically a surface block could be full and not at the edge of the bounds, but hopefully this isnt common enough to cause artifacts
    if (blocks_[block_index].non_empty_cell_count_ == 0 || (blocks_[block_index].non_empty_cell_count_ == NUM_CELLS_PER_BLOCK && !IsBlockAtEdge(block_index)))
    {
        return;
    }
    
    // The block is a surface block. Increment the count of surface blocks and store the index.
    uint surface_block_array_index;
    InterlockedAdd(surface_counts_[0].surface_blocks, 1, surface_block_array_index);
    surface_block_indices_[surface_block_array_index] = block_index;

    
    return;
}

groupshared uint block_index;

// Shader for detecting surface cells
[numthreads(NUM_CELLS_PER_AXIS_PER_BLOCK)]
void CSDetectSurfaceCellsMain(int3 group_index : SV_GroupID, int3 offset : SV_GroupThreadID, uint thread_index : SV_GroupIndex)
{
    
    // Load block index into groupshared memory
    if (thread_index == 0)
    {
        block_index = surface_block_indices_[group_index.x];
    }
    GroupMemoryBarrierWithGroupSync();
    
    // Use the block index to find the cell index
    uint cell_index = BlockIndexToCellIndex(block_index, offset);
    
    // If the cell is not completely empty and not completely full, it's a surface cell for certain
    uint particle_count = cells_[cell_index].particle_count_;
    if (particle_count > 0 && particle_count < CELL_MAX_PARTICLE_COUNT)
    {
        MarkAsSurfaceCell(cell_index);
        return;
    }

    // Otherwise must check if any neighbours are different 'fullness' (completely empty or completely full).
    // If so, this cell is a surface cell.
    [unroll]
    for (int i = -1; i < 2; i++)
    {
        [unroll]
        for (int j = -1; j < 2; j++)
        {
            [unroll]
            for (int k = -1; k < 2; k++)
            {
                // Don't check the current cell itself
                if (i == 0 && j == 0 && k == 0)
                {
                    continue;
                }
                
                bool neighbour_empty;
                bool neighbour_exists = IsNeighbourEmpty(cell_index, int3(i, j, k), neighbour_empty);
                
                // To be a surface cell, at least one neighbouring cell must be empty
                if (neighbour_exists && (particle_count == 0) ^ neighbour_empty)
                {
                    // The cell is a surface cell. Increment the count of surface cells and store the index.
                    MarkAsSurfaceCell(cell_index);
                    return;
                }
            }
        }
    }

}
// need to then send the count back to cpu so aabb buffer and 3d texture can be resized
// then compute the 3d texture

#endif