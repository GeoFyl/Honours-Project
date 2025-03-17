#ifndef GLOBAL_VALUES_HLSL
#define GLOBAL_VALUES_HLSL

#define NUM_PARTICLES 2
#define TEXTURE_RESOLUTION 256 

#define WORLD_MIN float3(0, 0, 0)
#define WORLD_MAX float3(1, 1, 1)

// Blocks: 4 x 4 x 4 total
// Cells: 16 x 16 x 16 total, 4 x 4 x 4 per block

#define NUM_CELLS 4096 
#define NUM_CELLS_PER_AXIS 16, 16, 16

#define NUM_BLOCKS 64 
#define NUM_BLOCKS_PER_AXIS 4, 4, 4

#define NUM_CELLS_PER_AXIS_PER_BLOCK 4, 4, 4
#define NUM_CELLS_PER_BLOCK 64

#define CELL_SIZE WORLD_MAX / float3(NUM_CELLS_PER_AXIS)
#define PARTICLE_RADIUS 0.0625

// Currently assumes cells are perfect cubes
// 2 x 2 x 2 = 8
#define BRICKS_PER_AXIS_PER_CELL 2
#define BRICKS_PER_CELL 8
#define BRICK_SIZE CELL_SIZE / float3(BRICKS_PER_AXIS_PER_CELL, BRICKS_PER_AXIS_PER_CELL, BRICKS_PER_AXIS_PER_CELL)

// Including voxels for adjacency data is 10, without is 8
#define VOXELS_PER_AXIS_PER_BRICK_ADJACENCY 10
#define VOXELS_PER_AXIS_PER_BRICK 8
#define VOXELS_PER_BRICK 512
#define VOXEL_SIZE BRICK_SIZE / float3(VOXELS_PER_AXIS_PER_BRICK, VOXELS_PER_AXIS_PER_BRICK, VOXELS_PER_AXIS_PER_BRICK)

#endif