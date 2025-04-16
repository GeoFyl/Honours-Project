#pragma once

// Fixed values
#define VOXELS_PER_AXIS_PER_BRICK 10


// Custom values
struct TestVariables {
	int num_particles_;
	int texture_res_;
	int num_cells_;
	int num_blocks_;
	int bricks_per_cell_;
	int padding;
};
extern TestVariables test_values_;

#define NUM_PARTICLES test_values_.num_particles_
#define TEXTURE_RESOLUTION test_values_.texture_res_
#define NUM_CELLS test_values_.num_cells_
#define NUM_BLOCKS test_values_.num_blocks_
#define BRICKS_PER_CELL test_values_.bricks_per_cell_


