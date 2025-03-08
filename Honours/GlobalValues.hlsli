#ifndef GLOBAL_VALUES_HLSL
#define GLOBAL_VALUES_HLSL

#define NUM_PARTICLES 125 // Also in ComputeStructs.h
#define TEXTURE_RESOLUTION 256 // Also in ComputeStructs.h

#define WORLD_MIN float3(0,0,0)
#define WORLD_MAX float3(2,2,2)

// Blocks: 4 x 4 x 4 total
// Cells: 16 x 16 x 16 total, 4 x 4 x 4 per block
#define NUM_CELLS 4096 // Also in ComputeStructs.h
#define NUM_BLOCKS 64 // Also in ComputeStructs.h
#define CELL_SIZE WORLD_MAX / 16.f;

#endif