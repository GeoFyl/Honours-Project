#ifndef GLOBAL_VALUES_HLSL
#define GLOBAL_VALUES_HLSL

#define NUM_PARTICLES 125 // Also in ComputeStructs.h
#define TEXTURE_RESOLUTION 256 // Also in ComputeStructs.h

#define WORLD_MIN float3(0, 0, 0)
#define WORLD_MAX float3(1, 1, 1)

// Blocks: 4 x 4 x 4 total
// Cells: 16 x 16 x 16 total, 4 x 4 x 4 per block

#define NUM_CELLS 4096 // Also in ComputeStructs.h
#define NUM_CELLS_PER_AXIS 16, 16, 16

#define NUM_BLOCKS 64 // Also in ComputeStructs.h
#define NUM_BLOCKS_PER_AXIS 4, 4, 4

#define NUM_CELLS_PER_AXIS_PER_BLOCK 4, 4, 4
#define NUM_CELLS_PER_BLOCK 64

#define CELL_SIZE WORLD_MAX / float3(NUM_CELLS_PER_AXIS);

#endif