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

struct SurfaceBlocks
{
    int count;
    int indices[NUM_BLOCKS];
};

struct SurfaceCells
{
    int count;
    int indices[NUM_CELLS];
};


RWStructuredBuffer<ParticlePosition> particle_positions_ : register(u0);
RWStructuredBuffer<Cell> cells_ : register(u1);
RWStructuredBuffer<Block> blocks_ : register(u2);
RWStructuredBuffer<SurfaceBlocks> surface_blocks_ : register(u3);
RWStructuredBuffer<SurfaceCells> surface_cells_ : register(u4);

// Based on http://www.gamedev.net/forums/topic/582945-find-grid-index-based-on-position/4709749/
int GetCellIndex(float3 particle_pos)
{
    // 16 x 16 x 16 cells
    float3 cell_size = WORLD_MAX / 16.f;
    
    int cell_ID = ((int) particle_pos.x / cell_size.x) + (((int) particle_pos.y / cell_size.y) * 16) + (((int) particle_pos.z / cell_size.z) * 256);
    
    return cell_ID;
}

// Works out the index of the block containing the given cell
int CellIndexToBlockIndex(int cell_index)
{
    int z = cell_index / 256;
    int y = (cell_index % 256) / 16;
    int x = cell_index % 16;
    
    return ((z / 4) * 16) + ((y / 4) * 4) + (x / 4);

}

// Works out the index of the cell from the block index and cell offset
int BlockIndexToCellIndex(int block_index, int3 cell_offset)
{
    // Convert block index to its (bx, by, bz) block coordinates
    int bz = block_index / 16;
    int by = (block_index % 16) / 4;
    int bx = block_index % 4;
    
    // Compute the absolute cell coordinates
    int x = bx * 4 + cell_offset.x;
    int y = by * 4 + cell_offset.y;
    int z = bz * 4 + cell_offset.z;
    
    // Turn this into an index
    return (z * 256) + (y * 16) + x;
}

// Finds a new cell index, given a current index and an offset
int OffsetCellIndex(int cell_index, int3 offset)
{
    int z = (cell_index / 256) + offset.z;
    int y = ((cell_index % 256) / 16) + offset.y;
    int x = (cell_index % 16) + offset.x;
    
    return (z * 256) + (y * 16) + x;
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
    int cell_index = GetCellIndex(particle_pos);
    
    // Increment the cell's particle count and assign intra-cell offset
    int particle_intra_cell_index;
    InterlockedAdd(cells_[cell_index].particle_count_, 1, particle_intra_cell_index);
    
    // Add the particle's index to cell's particle list
    cells_[cell_index].particle_indices_[particle_intra_cell_index] = dispatch_ID.x;
    
    // If this is the first particle in the cell, increment the blocks non empty cell counter
    if (particle_intra_cell_index == 0)
    {
        int block_index = CellIndexToBlockIndex(cell_index);
        InterlockedAdd(blocks_[block_index].non_empty_cell_count_, 1);
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
    
    int block_index = dispatch_ID.x;
    
    // If the cell count is 0 or 64 (block contains 64 cells), the block is not a surface block
    if (blocks_[block_index].non_empty_cell_count_ == 0 || blocks_[block_index].non_empty_cell_count_ == 64)
    {
        return;
    }
    
    // The block is a surface block. Increment the count of surface blocks and store the index.
    int surface_block_array_index;
    InterlockedAdd(surface_blocks_[0].count, 1, surface_block_array_index);
    surface_blocks_[0].indices[surface_block_array_index] = block_index;
    
    return;
}

// Can hopefully use workgraph to go from block detection ^ to cell detection:

// Shader for detecting surface cells
[numthreads(4, 4, 4)]
void CSDetectSurfaceCellsMain(int3 group_index : SV_GroupID, uint offset : SV_GroupThreadID)
{
    // Use the block index to find the cell index
    int block_index = surface_blocks_[0].indices[group_index.x];
    int cell_index = BlockIndexToCellIndex(block_index, offset);
    
    // To be a surface cell, the cell must not be empty
    if (cells_[cell_index].particle_count_ == 0)
    {
        return;
    }
    
    [unroll]
    for (int i = -1; i < 2; i++)
    {
        [unroll]
        for (int j = -1; j < 2; j++)
        {
            [unroll]
            for (int k = -1; k < 2; k++)
            {
                if (i == 0 && j == 0 && k == 0)
                {
                    continue;
                }
                
                // To be a surface cell, at least one neighbouring cell must be empty
                if (cells_[OffsetCellIndex(cell_index, int3(i, j, k))].particle_count_ == 0)
                {
                    // The cell is a surface cell. Increment the count of surface cells and store the index.
                    int surface_cells_array_index;
                    InterlockedAdd(surface_cells_[0].count, 1, surface_cells_array_index);
                    surface_cells_[0].indices[surface_cells_array_index] = cell_index;
                    return;
                }
            }
        }
    }

}
// need to then send the count back to cpu so aabb buffer and 3d texture can be resized
// then compute the 3d texture

#endif