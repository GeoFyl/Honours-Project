#pragma once
#include <string>


// Fixed values
#define VOXELS_PER_AXIS_PER_BRICK 10
#define NUM_CELLS 4096 
#define NUM_BLOCKS 64


// Custom values
enum SceneType {
	SceneRandom = 0,
	SceneGrid,
	SceneWave,
	SceneNormals
};

struct TestVariables {
	int num_particles_;
	int texture_res_;
	SceneType scene_;
};
extern TestVariables test_vars_;

#define NUM_PARTICLES test_vars_.num_particles_
#define TEXTURE_RESOLUTION test_vars_.texture_res_
#define SCENE test_vars_.scene_
#define BRICKS_PER_CELL ((TEXTURE_RESOLUTION / 128) * (TEXTURE_RESOLUTION / 128) * (TEXTURE_RESOLUTION / 128))

struct TestVariablesCPUOnly {
	float screen_res_[2];
	float view_dist_;
	bool test_mode_;
	std::string test_name_;
};
extern TestVariablesCPUOnly cpu_test_vars_;

#define VIEW_DIST cpu_test_vars_.view_dist_


