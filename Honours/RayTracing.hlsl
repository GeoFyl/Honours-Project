#ifndef RAYTRACING_HLSL
#define RAYTRACING_HLSL

#include "RayHelpers.hlsli"
#include "SdfHelpers.hlsli"

// Works out the index of the voxel from the brick index and voxel offset
uint3 BrickIndexToVoxelPosition(uint brick_index, uint3 voxel_offset)
{
    // Convert brick index to its (bx, by, bz) brick coordinates
    uint3 brick_pool_dimensions = comp_constant_buffer_.brick_pool_dimensions_;
    
    uint bricks_per_z = brick_pool_dimensions.x * brick_pool_dimensions.y;
    uint bz = brick_index / bricks_per_z;
    uint by = (brick_index % bricks_per_z) / brick_pool_dimensions.x;
    uint bx = brick_index % brick_pool_dimensions.x;
    
    // Compute the voxel position
    uint x = bx * VOXELS_PER_AXIS_PER_BRICK + voxel_offset.x;
    uint y = by * VOXELS_PER_AXIS_PER_BRICK + voxel_offset.y;
    uint z = bz * VOXELS_PER_AXIS_PER_BRICK + voxel_offset.z;
    
    // Turn this into an index
    return uint3(x, y, z);
}

float3 BrickIndexToBrickPoolUVW(uint brick_index, uint3 voxel_offset)
{
    float3 uvw = BrickIndexToVoxelPosition(PrimitiveIndex(), voxel_offset) + float3(0.5f, 0.5f, 0.5f);
                    
    uvw /= (float3) comp_constant_buffer_.brick_pool_dimensions_ * float3(VOXELS_PER_AXIS_PER_BRICK, VOXELS_PER_AXIS_PER_BRICK, VOXELS_PER_AXIS_PER_BRICK);
    
    return uvw;
}



// ------------ Ray Generation Shader ----------------

[shader("raygeneration")]
void RayGenerationShader()
{
	Ray ray;

    // Generate ray from camera into the scene
	GenerateCameraRay(DispatchRaysIndex().xy, ray.origin_, ray.direction_);
	RayPayload payload = TracePrimaryRay(ray, 0);

    // Write the raytraced color to the output texture.
	render_target_[DispatchRaysIndex().xy] = payload.colour_;
}


// -------------- Intersection Shader ----------------

[shader("intersection")]
void IntersectionShader()
{
    if (!(rt_constant_buffer_.rendering_flags_ & RENDERING_FLAG_VISUALIZE_AABBS) &&
        rt_constant_buffer_.rendering_flags_ & RENDERING_FLAG_VISUALIZE_PARTICLES)
    {
        if (RenderParticlesVisualized())
        {
            RayIntersectionAttributes attributes;
            ReportHit(1, 0, attributes);
        }
        return;
    }
    else
    {
        float3 aabb[2] = { AABBs_[PrimitiveIndex()].min_, AABBs_[PrimitiveIndex()].max_ };
        
        // Construct ray, translated to origin
        Ray ray;
        ray.origin_ = WorldRayOrigin();
        ray.direction_ = WorldRayDirection();
    
        float t_min, t_max;
        if (RayAABBIntersectionTest(ray, aabb, t_min, t_max))
        {
            // --------------------------------------------------------------------------
            // We may want to just render the AABBs
            if (rt_constant_buffer_.rendering_flags_ & RENDERING_FLAG_VISUALIZE_AABBS)
            {
                ray.origin_ -= aabb[0];                   
                float3 aabb_uvw = ray.origin_ + max(t_min, 0) * ray.direction_;
                aabb_uvw /= BRICK_SIZE;
                
                RayIntersectionAttributes attributes;
                attributes.float_3_ = aabb_uvw;
                
                ReportHit(max(t_min, RayTMin()), 0, attributes);
                return;
            }
            
            // --------------------------------------------------------------------------
            
            if (!(rt_constant_buffer_.rendering_flags_ & RENDERING_FLAG_SIMPLE_AABB) &&
                !(rt_constant_buffer_.rendering_flags_ & RENDERING_FLAG_ANALYTICAL))
            {
                ray.origin_ -= aabb[0];
            }
            
            // Perform sphere tracing through the AABB.
            uint i = 0;
            while (i++ < MAX_SPHERE_TRACING_STEPS && t_min <= t_max)
            {
                float3 position = ray.origin_ + max(t_min, 0) * ray.direction_;

                // If we are using the complex AABBs and texture
                if (!(rt_constant_buffer_.rendering_flags_ & RENDERING_FLAG_SIMPLE_AABB) &&
                    !(rt_constant_buffer_.rendering_flags_ & RENDERING_FLAG_ANALYTICAL))
                {
                    position /= BRICK_SIZE;
                    
                    //float3 voxel_offset = (position / BRICK_SIZE) * VOXELS_PER_AXIS_PER_BRICK;
                    uint3 voxel_offset = position * VOXELS_PER_AXIS_PER_BRICK;
                    
                    // Need to clamp voxel offset to be in correct range. clamp() wasn't working for some reason
                    uint max_offset = VOXELS_PER_AXIS_PER_BRICK - 1;
                    if (voxel_offset.x > max_offset)
                        voxel_offset.x = max_offset;
                    if (voxel_offset.y > max_offset)
                        voxel_offset.y = max_offset;
                    if (voxel_offset.z > max_offset)
                        voxel_offset.z = max_offset;

                    position = BrickIndexToBrickPoolUVW(PrimitiveIndex(), voxel_offset);

                }
                else if (!(rt_constant_buffer_.rendering_flags_ & RENDERING_FLAG_ANALYTICAL))
                {
                    position /= WORLD_MAX;
                }
               
                //float distance = GetDistance(position);
                //float distance = sdf_texture_.Load(int4(position, 0));
                float distance = sdf_texture_.SampleLevel(sampler_, position, 0);
                
                //----------
                RayIntersectionAttributes attributes;
                attributes.float_3_ = float3(distance, 0, 0);
                //attributes.float_3_ = position;
                    
                ReportHit(max(t_min, RayTMin()), 0, attributes);
                return;
                // ----------

                // Has the ray intersected the primitive? 
                if (distance <= SPHERE_TRACING_THRESHOLD)
                {
                    // Store the normal and report hit                    
                    RayIntersectionAttributes attributes;
                    attributes.float_3_ = CalculateNormal(position);
                    
                    ReportHit(max(t_min, RayTMin()), 0, attributes);
                    return;
                }

                // Since distance is the minimum distance to the primitive, 
                // we can safely jump by that amount without intersecting the primitive.
                t_min += distance;
            }
        }
    }
}


// -------------- Closest Hit Shader ----------------

[shader("closesthit")]
void ClosestHitShader(inout RayPayload payload, in RayIntersectionAttributes attributes)
{
    payload.colour_ = float4(attributes.float_3_, 1.f);
    return;
    
    //if (rt_constant_buffer_.rendering_flags_ & RENDERING_FLAG_VISUALIZE_AABBS)
    //{
    //    payload.colour_ = float4(attributes.float_3_, 1.f);
    //    return;
    //}
    //else if (rt_constant_buffer_.rendering_flags_ & RENDERING_FLAG_VISUALIZE_PARTICLES)
    //{
    //    payload.colour_ = float4(0.306, 0.941, 0.933, 1);
    //    return;
    //}
    //else if (rt_constant_buffer_.rendering_flags_ & RENDERING_FLAG_NORMALS)
    //{
    //    payload.colour_ = float4(attributes.float_3_, 1.f);
    //    return;
    //}
    //else
    //{
    //    //float3 position = WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
    //    //float4 specular = float4(0.9f, 0.9f, 0.9f, 1);
    //    //payload.colour_ = float4(0.306, 0.941, 0.933, 1) * CalculateLighting(attributes.float_3_, rt_constant_buffer_.camera_pos_, position, specular) + specular;
        
    //    payload.colour_ = float4(0.306, 0.941, 0.933, 1);

    //}
}


// -------------- Miss Shader ----------------
[shader("miss")]
void MissShader(inout RayPayload payload)
{
    payload.colour_ = float4(0.0f, 0.2f, 0.4f, 1.0f);
}


#endif // RAYTRACING_HLSL