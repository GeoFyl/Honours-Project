#ifndef COMPUTE_GRID_COMMON_HLSL
#define COMPUTE_GRID_COMMON_HLSL

#include "ComputeCommon.hlsli"

#define CELL_MAX_PARTICLE_COUNT 8 // Also in ComputeStructs.h

// ---- Two-level Grid -----
struct Cell
{
    uint particle_count_;
    uint particle_indices_[CELL_MAX_PARTICLE_COUNT];
};

struct Block
{
    uint non_empty_cell_count_;
};

struct GridSurfaceCounts
{
    uint surface_blocks;
    uint surface_cells;
};

// Based on http://www.gamedev.net/forums/topic/582945-find-grid-index-based-on-position/4709749/
int GetCellIndex(float3 particle_pos)
{
    uint3 cells_per_axis = uint3(NUM_CELLS_PER_AXIS);
    
    float3 cell_size = WORLD_MAX / cells_per_axis;
    
    uint cell_ID = (uint) (particle_pos.x / cell_size.x) + ((uint) (particle_pos.y / cell_size.y) * cells_per_axis.x) + ((uint) (particle_pos.z / cell_size.z) * cells_per_axis.x * cells_per_axis.y);
    
    return cell_ID;
}

uint3 CellIndexTo3DCoords(uint cell_index)
{
    uint3 cells_per_axis = uint3(NUM_CELLS_PER_AXIS);
    uint3 coords = uint3(0,0,0);
    
    uint cells_per_z = cells_per_axis.x * cells_per_axis.y;
    coords.z = cell_index / cells_per_z;
    coords.y = (cell_index % cells_per_z) / cells_per_axis.x;
    coords.x = cell_index % cells_per_axis.x;
    
    return coords;
}

// Works out the index of the block containing the given cell
int CellIndexToBlockIndex(uint cell_index)
{
    uint3 coords = CellIndexTo3DCoords(cell_index);
    
    uint3 cells_per_block = uint3(NUM_CELLS_PER_AXIS_PER_BLOCK);
    uint3 blocks_per_axis = uint3(NUM_BLOCKS_PER_AXIS);
    
    return ((coords.z / cells_per_block.z) * blocks_per_axis.x * blocks_per_axis.y) + ((coords.y / cells_per_block.y) * blocks_per_axis.x) + (coords.x / cells_per_block.x);
}

// Works out the index of the cell from the block index and cell offset
int BlockIndexToCellIndex(uint block_index, uint3 cell_offset)
{
    // Convert block index to its (bx, by, bz) block coordinates
    uint3 blocks_per_axis = uint3(NUM_BLOCKS_PER_AXIS);
    uint blocks_per_z = blocks_per_axis.x * blocks_per_axis.y;
    uint bz = block_index / blocks_per_z;
    uint by = (block_index % blocks_per_z) / blocks_per_axis.x;
    uint bx = block_index % blocks_per_axis.x;
    
    // Compute the absolute cell coordinates
    uint3 cells_per_block = uint3(NUM_CELLS_PER_AXIS_PER_BLOCK);
    uint x = bx * cells_per_block.x + cell_offset.x;
    uint y = by * cells_per_block.y + cell_offset.y;
    uint z = bz * cells_per_block.z + cell_offset.z;
    
    // Turn this into an index
    uint3 cells_per_axis = uint3(NUM_CELLS_PER_AXIS);
    return (z * cells_per_axis.x * cells_per_axis.y) + (y * cells_per_axis.x) + x;
}

bool IsBlockAtEdge(uint block_index)
{
    // Convert block index to its (bx, by, bz) block coordinates
    uint3 blocks_per_axis = uint3(NUM_BLOCKS_PER_AXIS);
    uint blocks_per_z = blocks_per_axis.x * blocks_per_axis.y;
    
    uint bz = block_index / blocks_per_z;
    if (bz == 0 || bz == blocks_per_axis.z - 1)
    {
        return true;
    }
    uint by = (block_index % blocks_per_z) / blocks_per_axis.x;
    if (by == 0 || by == blocks_per_axis.y - 1)
    {
        return true;
    }
    uint bx = block_index % blocks_per_axis.x;
    if (bx == 0 || bx == blocks_per_axis.x - 1)
    {
        return true;
    }
    return false;
}

#endif