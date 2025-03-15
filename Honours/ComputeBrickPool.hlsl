#ifndef COMPUTE_TEX_HLSL
#define COMPUTE_TEX_HLSL

#include "ComputeCommon.hlsli"
#include "SdfHelpers.hlsli"

RWTexture3D<snorm float> output_texture_ : register(u0);
StructuredBuffer<AABB> aabbs_ : register(t1);
ConstantBuffer<ComputeCB> constant_buffer_ : register(b0);

groupshared AABB aabb;
groupshared uint3 brick_pool_dimensions;

// Works out the index of the voxel from the brick index and voxel offset
uint3 BrickIndexToVoxelPosition(uint brick_index, uint3 voxel_offset)
{
    // Convert brick index to its (bx, by, bz) brick coordinates
   // uint3 bricks_per_axis = uint3(NUM_BLOCKS_PER_AXIS);
    uint bricks_per_z = brick_pool_dimensions.x * brick_pool_dimensions.y;
    uint bz = brick_index / bricks_per_z;
    uint by = (brick_index % bricks_per_z) / brick_pool_dimensions.x;
    uint bx = brick_index % brick_pool_dimensions.x;
    
    // Compute the voxel position
    //uint3 cells_per_block = uint3(NUM_CELLS_PER_AXIS_PER_BLOCK);
    uint x = bx * VOXELS_PER_AXIS_PER_BRICK + voxel_offset.x;
    uint y = by * VOXELS_PER_AXIS_PER_BRICK + voxel_offset.y;
    uint z = bz * VOXELS_PER_AXIS_PER_BRICK + voxel_offset.z;
    
    // Turn this into an index
    //uint3 cells_per_axis = uint3(NUM_CELLS_PER_AXIS);
    return uint3(x, y, z);
}

// Shader for creating SDF 3D texture 
[numthreads(VOXELS_PER_AXIS_PER_BRICK, VOXELS_PER_AXIS_PER_BRICK, VOXELS_PER_AXIS_PER_BRICK)]
void CSBrickPoolMain(int3 brick_index : SV_GroupID, int3 voxel_offset : SV_GroupThreadID, uint voxel_index : SV_GroupIndex)
{
    //float tex_res = TEXTURE_RESOLUTION - 1.0f;
    //float3 position = lerp(WORLD_MIN, WORLD_MAX, dispatch_ID / float3(tex_res, tex_res, tex_res));
    //output_texture_[dispatch_ID] = GetAnalyticalSignedDistance(position);
    
    // Load this bricks AABB into groupshared memory
    if (voxel_index == 0)
    {
        aabb = aabbs_[brick_index.x];
        brick_pool_dimensions = constant_buffer_.brick_pool_dimensions_;
    }
    GroupMemoryBarrierWithGroupSync();
    
    float3 voxel_size = VOXEL_SIZE;
    float3 position = aabb.min_ + (voxel_size * (float3)voxel_offset) + (voxel_size * 0.5f);
    
    //output_texture_[BrickIndexToVoxelPosition(brick_index.x, voxel_offset)] = GetAnalyticalSignedDistance(position);
    output_texture_[BrickIndexToVoxelPosition(brick_index.x, voxel_offset)] = position.x;
}

#endif