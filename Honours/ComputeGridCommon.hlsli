#ifndef COMPUTE_GRID_COMMON_HLSL
#define COMPUTE_GRID_COMMON_HLSL

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

//struct SurfaceBlocks
//{
//    uint count;
//    uint indices[NUM_BLOCKS];
//};

//struct SurfaceCells
//{
//    uint count;
//    uint indices[NUM_CELLS];
//};


#endif