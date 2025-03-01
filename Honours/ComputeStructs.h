#pragma once
#include <DirectXMath.h>

#define NUM_PARTICLES 125 // Also in ComputeCommon.hlsli, RayTracingCommon.hlsli
#define TEXTURE_RESOLUTION 256 // Also in ComputeCommon.hlsli, RayTracingCommon.hlsli


#define CELL_MAX_PARTICLE_COUNT 8 // Also in ComputeGrid.hlsl
#define NUM_CELLS 4096 // Also in ComputeGrid.hlsl
#define NUM_BLOCKS 64 // Also in ComputeGrid.hlsl

using namespace DirectX;

struct ParticlePosition {
	XMFLOAT3 position_;
	float speed_;
	float start_y_;
};

struct ComputeCB {
	float time_ = 0;
};

// ---- Two-level Grid -----
struct Cell
{
	unsigned int particle_count_;
	unsigned int particle_indices_[CELL_MAX_PARTICLE_COUNT];
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

