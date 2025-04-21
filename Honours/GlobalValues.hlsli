#ifndef GLOBAL_VALUES_HLSL
#define GLOBAL_VALUES_HLSL

//#define NUM_PARTICLES 200
//#define TEXTURE_RESOLUTION 256 

#define WORLD_MIN float3(0, 0, 0)
#define WORLD_MAX float3(1, 1, 1)

// Blocks: 4 x 4 x 4 total
// Cells: 16 x 16 x 16 total, 4 x 4 x 4 per block

//#define NUM_CELLS 4096 
#define NUM_CELLS_PER_AXIS 16, 16, 16

//#define NUM_BLOCKS 64 
#define NUM_BLOCKS_PER_AXIS 4, 4, 4

#define NUM_CELLS_PER_AXIS_PER_BLOCK 4, 4, 4
#define NUM_CELLS_PER_BLOCK 64

// cell size >= influence radius
// influence radius = 2 * particle radius
#define CELL_SIZE WORLD_MAX / float3(NUM_CELLS_PER_AXIS)
#define PARTICLE_INFLUENCE_RADIUS 0.0625
#define PARTICLE_RADIUS 0.03125


// Currently assumes cells are perfect cubes
// 2 x 2 x 2 = 8
#define BRICKS_PER_AXIS_PER_CELL 2
//#define BRICKS_PER_CELL 8
#define BRICK_SIZE CELL_SIZE / float3(BRICKS_PER_AXIS_PER_CELL, BRICKS_PER_AXIS_PER_CELL, BRICKS_PER_AXIS_PER_CELL)

// Including voxels for adjacency data is 10, without is 8
#define VOXELS_PER_AXIS_PER_BRICK 10
#define CORE_VOXELS_PER_AXIS_PER_BRICK 8
#define CORE_VOXELS_PER_BRICK 512
#define ADJACENCY_VOXELS_PER_BRICK_TOP_BOTTOM 10, 10
#define ADJACENCY_VOXELS_PER_BRICK_LEFT_RIGHT 10, 8
#define ADJACENCY_VOXELS_PER_BRICK_FRONT_BACK 8, 8
#define VOXEL_SIZE BRICK_SIZE / float3(CORE_VOXELS_PER_AXIS_PER_BRICK, CORE_VOXELS_PER_AXIS_PER_BRICK, CORE_VOXELS_PER_AXIS_PER_BRICK)


// Customizable values for testing

#define SceneRandom 0
#define SceneGrid 1
#define SceneWave 2
#define SceneNormals 3

struct TestVariables
{
    int num_particles_;
    int texture_res_;
    int num_cells_;
    int num_blocks_;
    int bricks_per_cell_;
    int scene_;
};
ConstantBuffer<TestVariables> test_values_ : register(b0);

#define NUM_PARTICLES test_values_.num_particles_
#define TEXTURE_RESOLUTION test_values_.texture_res_
#define NUM_CELLS test_values_.num_cells_
#define NUM_BLOCKS test_values_.num_blocks_
#define BRICKS_PER_CELL test_values_.bricks_per_cell_
#define SCENE test_values_.scene_

#endif