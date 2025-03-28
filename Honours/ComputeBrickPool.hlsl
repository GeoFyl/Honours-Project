#ifndef COMPUTE_TEX_HLSL
#define COMPUTE_TEX_HLSL

#include "ComputeGridCommon.hlsli"
#include "SdfHelpers.hlsli"

RWTexture3D<snorm float> output_texture_ : register(u0);
StructuredBuffer<AABB> aabbs_ : register(t1);
StructuredBuffer<Cell> cell_particle_counts_ : register(t2);
StructuredBuffer<uint> cell_global_index_offsets_ : register(t3);
ConstantBuffer<ComputeCB> constant_buffer_ : register(b0);

groupshared AABB aabb;
groupshared uint3 brick_pool_dimensions;
groupshared uint neighbouring_cells[27];

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
    uint x = bx * VOXELS_PER_AXIS_PER_BRICK_ADJACENCY + voxel_offset.x;
    uint y = by * VOXELS_PER_AXIS_PER_BRICK_ADJACENCY + voxel_offset.y;
    uint z = bz * VOXELS_PER_AXIS_PER_BRICK_ADJACENCY + voxel_offset.z;
    
    // Turn this into an index
    //uint3 cells_per_axis = uint3(NUM_CELLS_PER_AXIS);
    return uint3(x, y, z);
}

float GetSignedDistanceNNS(float3 position)
{
    // vector between the particle position and the current sphere tracing position
    
    //float distance = GetDistanceToSphere(particles_[neighbouring_cells[0]].position_ - position, PARTICLE_RADIUS);
    float distance;
    bool first_particle_found = false;
    
    for (int x = 0; x < 27; x++)
    {
        int cell_index = neighbouring_cells[x];
        if (cell_index > -1)
        {
            uint particle_count = cell_particle_counts_[cell_index].particle_count_;
            uint cell_global_index_offset = cell_global_index_offsets_[cell_index];
            for (uint i = 0; i < particle_count; i++)
            {
                float distance1 = GetDistanceToSphere(particles_[cell_global_index_offset + i].position_ - position, PARTICLE_RADIUS);
               // if (distance1 <= PARTICLE_RADIUS)
               // {
                    if (!first_particle_found)
                    {
                        first_particle_found = true;
                        distance = distance1;
                    }
                    else
                    {
                        distance = SmoothMin(distance, distance1, PARTICLE_RADIUS);
                        //distance = min(distance, distance1);                           
                    }
               // }
            }
        }
    }
    
    return distance;
}

// Shader for creating SDF 3D texture 
[numthreads(VOXELS_PER_AXIS_PER_BRICK_ADJACENCY, VOXELS_PER_AXIS_PER_BRICK_ADJACENCY, VOXELS_PER_AXIS_PER_BRICK_ADJACENCY)]
void CSBrickPoolMain(int3 brick_index : SV_GroupID, int3 voxel_offset : SV_GroupThreadID, uint voxel_index : SV_GroupIndex)
{
    // Load this bricks AABB into groupshared memory
    if (voxel_index == 0)
    {
        aabb = aabbs_[brick_index.x];
        brick_pool_dimensions = constant_buffer_.brick_pool_dimensions_;
    }
    
    // Load list of indices of neighbouring cells into groupshared memory
    if (voxel_offset.x <= 2 && voxel_offset.y <= 2 && voxel_offset.z <= 2)
    {
        int neighbour_list_index = (voxel_offset.z * 9) + (voxel_offset.y * 3) + voxel_offset.x;
        neighbouring_cells[neighbour_list_index] = OffsetCellIndex(brick_index.x / BRICKS_PER_CELL, voxel_offset - 1);
    }
    
    GroupMemoryBarrierWithGroupSync();
    
    float3 voxel_size = VOXEL_SIZE;
    float3 position = aabb.min_ + (voxel_size * (float3) (voxel_offset - 1)) + (voxel_size * 0.5f); // voxel offset is offset by -(1,1,1) to account for adjacency voxels
    
    output_texture_[BrickIndexToVoxelPosition(brick_index.x, voxel_offset)] = GetSignedDistanceNNS(position);
    
}

#endif