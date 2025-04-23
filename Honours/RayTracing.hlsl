#ifndef RAYTRACING_HLSL
#define RAYTRACING_HLSL

#include "RayHelpers.hlsli"
#include "SdfHelpers.hlsli"

// Works out the coords of the voxel within the brick pool from the brick index
uint3 BrickIndexToVoxelPosition(uint brick_index)
{
    // Convert brick index to its (bx, by, bz) brick coordinates
    uint3 brick_pool_dimensions = comp_constant_buffer_.brick_pool_dimensions_;
    
    uint bricks_per_z = brick_pool_dimensions.x * brick_pool_dimensions.y;
    uint bz = brick_index / bricks_per_z;
    uint by = (brick_index % bricks_per_z) / brick_pool_dimensions.x;
    uint bx = brick_index % brick_pool_dimensions.x;
    
    // Compute the voxel position
    uint x = bx;
    uint y = by;
    uint z = bz;
    
    // Turn this into an index
    return uint3(x, y, z);
}

// Work out the uvw within the brick pool using the current brick index (primitive index) 
// and the coords of the voxel within the brick
float3 BrickIndexToBrickPoolUVW(float3 voxel_offset)
{
    float3 uvw = (voxel_offset / VOXELS_PER_AXIS_PER_BRICK) + BrickIndexToVoxelPosition(PrimitiveIndex());
                    
    uvw /= (float3) comp_constant_buffer_.brick_pool_dimensions_;
    
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
            float3 position;
            float3 voxel_offset = float3(0,0,0);
            uint i = 0;
            while (i++ < MAX_SPHERE_TRACING_STEPS && t_min <= t_max)
            {
                position = clamp(ray.origin_ + max(t_min, 0) * ray.direction_, WORLD_MIN, WORLD_MAX);
                
                // If we are using the complex AABBs and brick pool
                if (!(rt_constant_buffer_.rendering_flags_ & RENDERING_FLAG_SIMPLE_AABB) &&
                    !(rt_constant_buffer_.rendering_flags_ & RENDERING_FLAG_ANALYTICAL))
                {
                    // Find voxel offset from position within the brick
                    position /= BRICK_SIZE;
                    voxel_offset = position * CORE_VOXELS_PER_AXIS_PER_BRICK;
                    
                    // position variable is now the uvw to sample the brick pool
                    position = BrickIndexToBrickPoolUVW(voxel_offset + 1.f); // voxel offset is offset by (1,1,1) to account for adjacency voxels

                }
                else if (!(rt_constant_buffer_.rendering_flags_ & RENDERING_FLAG_ANALYTICAL))
                {
                    // We are using the simple SDF 3D texture
                    
                    // position variable is now the uvw to sample the texture
                    position /= WORLD_MAX;               
                }
               
                // Get the SDF value for the current position
                float distance = GetDistance(position);

                // Has the ray intersected the isosurface? 
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
    if (rt_constant_buffer_.rendering_flags_ & RENDERING_FLAG_VISUALIZE_AABBS)
    {
        payload.colour_ = float4(attributes.float_3_, 1.f);
        return;
    }
    else if (rt_constant_buffer_.rendering_flags_ & RENDERING_FLAG_VISUALIZE_PARTICLES)
    {
        payload.colour_ = float4(0.306, 0.941, 0.933, 1);
        return;
    }
    else if (rt_constant_buffer_.rendering_flags_ & RENDERING_FLAG_NORMALS)
    {
        payload.colour_ = float4(attributes.float_3_, 1.f);
        return;
    }
    else
    {
        float3 position = WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
        float4 specular = float4(0.9f, 0.9f, 0.9f, 1);
        payload.colour_ = float4(0.306, 0.941, 0.933, 1) * CalculateLighting(attributes.float_3_, rt_constant_buffer_.camera_pos_, position, specular) + specular;
    }
}


// -------------- Miss Shader ----------------
[shader("miss")]
void MissShader(inout RayPayload payload)
{
    payload.colour_ = float4(0.0f, 0.2f, 0.4f, 1.0f);
}


#endif // RAYTRACING_HLSL