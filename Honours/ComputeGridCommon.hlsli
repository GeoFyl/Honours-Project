#ifndef COMPUTE_GRID_COMMON_HLSL
#define COMPUTE_GRID_COMMON_HLSL

#include "ComputeCommon.hlsli"

#define CELL_MAX_PARTICLE_COUNT 8 // Also in ComputeStructs.h

// ---- Two-level Grid -----
struct Cell
{
    uint particle_count_;
    //uint particle_indices_[CELL_MAX_PARTICLE_COUNT];
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

static const int invalid_block_indices[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };

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

uint Cell3DCoordsToBlockIndex(uint3 coords, int3 block_offset)
{
    uint3 cells_per_block = uint3(NUM_CELLS_PER_AXIS_PER_BLOCK);
    uint3 blocks_per_axis = uint3(NUM_BLOCKS_PER_AXIS);
    
    int bx = (coords.x / cells_per_block.x) + block_offset.x;
    int by = (coords.y / cells_per_block.y) + block_offset.y;
    int bz = (coords.z / cells_per_block.z) + block_offset.z;
    
    return (bz * blocks_per_axis.x * blocks_per_axis.y) + (by * blocks_per_axis.x) + bx;
}


// Works out the index of the block containing the given cell, and blocks neighbouring the cell
void CellIndexToBlockIndices(uint cell_index, out int block_indices[8])
{
    block_indices = invalid_block_indices;
    uint3 cells_per_block = uint3(NUM_CELLS_PER_AXIS_PER_BLOCK);
    uint3 coords = CellIndexTo3DCoords(cell_index);
    
    int4 block_offset = int4(0, 0, 0, 0);
    
    int3 intra_block_coords = coords % cells_per_block;
    
    block_indices[0] = Cell3DCoordsToBlockIndex(coords, block_offset.xyz);
    
    if (intra_block_coords.x == 0)
    {
        block_offset.x = -1;
        block_indices[1] = Cell3DCoordsToBlockIndex(coords, block_offset.xww);
    }
    else if (intra_block_coords.x == NUM_CELLS_PER_AXIS_PER_BLOCK - 1)
    {
        block_offset.x = -1;
        block_indices[1] = Cell3DCoordsToBlockIndex(coords, block_offset.xww);
    }
    if (intra_block_coords.y == 0)
    {
        block_offset.y = -1;
        block_indices[2] = Cell3DCoordsToBlockIndex(coords, block_offset.wyw);
    }
    else if (intra_block_coords.y == NUM_CELLS_PER_AXIS_PER_BLOCK - 1)
    {
        block_offset.y = -1;
        block_indices[2] = Cell3DCoordsToBlockIndex(coords, block_offset.wyw);
    }
    if (intra_block_coords.z == 0)
    {
        block_offset.z = -1;
        block_indices[3] = Cell3DCoordsToBlockIndex(coords, block_offset.wwz);
    }
    else if (intra_block_coords.z == NUM_CELLS_PER_AXIS_PER_BLOCK - 1)
    {
        block_offset.z = -1;
        block_indices[3] = Cell3DCoordsToBlockIndex(coords, block_offset.wwz);
    }

    if (block_offset.x && block_offset.y)
        block_indices[4] = Cell3DCoordsToBlockIndex(coords, block_offset.xyw);
    if (block_offset.y && block_offset.z)
        block_indices[5] = Cell3DCoordsToBlockIndex(coords, block_offset.wyz);
    if (block_offset.x && block_offset.z)
        block_indices[6] = Cell3DCoordsToBlockIndex(coords, block_offset.xwz);
    if (block_offset.x && block_offset.y && block_offset.z)
        block_indices[7] = Cell3DCoordsToBlockIndex(coords, block_offset.xyz);
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