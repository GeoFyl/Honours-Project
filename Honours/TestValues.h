#pragma once

// Fixed values
#define VOXELS_PER_AXIS_PER_BRICK 10


// Custom values
enum SceneType {
	SceneRandom = 0,
	SceneGrid,
	SceneWave
};

struct TestVariables {
	int num_particles_;
	int texture_res_;
	int num_cells_;
	int num_blocks_;
	int bricks_per_cell_;
	SceneType scene_;
};
extern TestVariables test_vars_;

#define NUM_PARTICLES test_vars_.num_particles_
#define TEXTURE_RESOLUTION test_vars_.texture_res_
#define NUM_CELLS test_vars_.num_cells_
#define NUM_BLOCKS test_vars_.num_blocks_
#define BRICKS_PER_CELL test_vars_.bricks_per_cell_
#define SCENE test_vars_.scene_

struct TestVariablesCPUOnly {
	float screen_res_[2];
	float view_dist_;
};
extern TestVariablesCPUOnly cpu_test_vars_;

#define VIEW_DIST cpu_test_vars_.view_dist_


