#ifndef COMPUTE_GRID_COMMON_HLSL
#define COMPUTE_GRID_COMMON_HLSL

#include "ComputeCommon.hlsli"

#define CELL_MAX_PARTICLE_COUNT 8 // Also in ComputeStructs.h
#define NUM_CELLS 4096 // Also in ComputeStructs.h
#define NUM_BLOCKS 64 // Also in ComputeStructs.h

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


// Blocks: 4 x 4 x 4 total
// Cells: 16 x 16 x 16 total, 4 x 4 x 4 per block

// Based on http://www.gamedev.net/forums/topic/582945-find-grid-index-based-on-position/4709749/
int GetCellIndex(float3 particle_pos)
{
    // 16 x 16 x 16 cells
    float3 cell_size = WORLD_MAX / 16.f;
    
    uint cell_ID = (uint)(particle_pos.x / cell_size.x) + ((uint)(particle_pos.y / cell_size.y) * 16) + ((uint)(particle_pos.z / cell_size.z) * 256);
    
    return cell_ID;
}

uint3 CellIndexTo3DCoords(uint cell_index)
{
    uint3 coords = uint3(0,0,0);
    
    coords.z = cell_index / 256;
    coords.y = (cell_index % 256) / 16;
    coords.x = cell_index % 16;
    
    return coords;
}

// Works out the index of the block containing the given cell
int CellIndexToBlockIndex(uint cell_index)
{
    uint3 coords = CellIndexTo3DCoords(cell_index);
    
    return ((coords.z / 4) * 16) + ((coords.y / 4) * 4) + (coords.x / 4);
}

// Works out the index of the cell from the block index and cell offset
int BlockIndexToCellIndex(uint block_index, uint3 cell_offset)
{
    // Convert block index to its (bx, by, bz) block coordinates
    uint bz = block_index / 16;
    uint by = (block_index % 16) / 4;
    uint bx = block_index % 4;
    
    // Compute the absolute cell coordinates
    uint x = bx * 4 + cell_offset.x;
    uint y = by * 4 + cell_offset.y;
    uint z = bz * 4 + cell_offset.z;
    
    // Turn this into an index
    return (z * 256) + (y * 16) + x;
}

bool IsBlockAtEdge(uint block_index)
{
     // Convert block index to its (bx, by, bz) block coordinates
    uint bz = block_index / 16;
    if (bz == 0 || bz == 3)
    {
        return true;
    }
    uint by = (block_index % 16) / 4;
    if (by == 0 || by == 3)
    {
        return true;
    }
    uint bx = block_index % 4;
    if (bx == 0 || bx == 3)
    {
        return true;
    }
    return false;
}

#endif