#pragma once
#include <DirectXMath.h>
#include "TestValues.h"

using namespace DirectX;

struct ParticleData {
	XMFLOAT3 position_;
	float speed_;
	float start_y_;
	UINT32 intra_cell_index_;
	UINT32 cell_index_;
};

struct ComputeCB {
	float time_ = 0;
	XMUINT3 brick_pool_dimensions_;
};

// ---- Two-level Grid -----
struct Cell
{
	unsigned int particle_count_;
	//unsigned int particle_indices_[CELL_MAX_PARTICLE_COUNT];
};

struct Block
{
	unsigned int non_empty_cell_count_;
};

struct GridSurfaceCounts {
	unsigned int surface_blocks;
	unsigned int surface_cells;
};

//struct SurfaceBlocks {
//	unsigned int count;
//	unsigned int indices[NUM_BLOCKS];
//};
//
//struct SurfaceCells {
//	unsigned int count;
//	unsigned int indices[NUM_CELLS];
//};

