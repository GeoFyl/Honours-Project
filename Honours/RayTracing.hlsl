#ifndef RAYTRACING_HLSL
#define RAYTRACING_HLSL

#include "RayHelpers.hlsli"
#include "SdfHelpers.hlsli"

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
    
    if (constant_buffer_.rendering_flags_ & RENDERING_FLAG_VISUALIZE_AABBS)
    {
        float3 aabb[2] = { AABBs_[PrimitiveIndex()].min_, AABBs_[PrimitiveIndex()].max_ };

        // Construct ray, translated to origin
        Ray ray;
        ray.origin_ = WorldRayOrigin();
        ray.direction_ = WorldRayDirection();
        
        float t_min, t_max;
        if (RayAABBIntersectionTest(ray, aabb, t_min, t_max))
        {
            RayIntersectionAttributes attributes;
            ReportHit(max(t_min, RayTMin()), 0, attributes);
            return;            
        }

    }
    else if (constant_buffer_.rendering_flags_ & RENDERING_FLAG_VISUALIZE_PARTICLES)
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
        //const AABB aabb = AABBs_[PrimitiveIndex()];
        
        // Construct ray, translated to origin
        Ray ray;
        ray.origin_ = WorldRayOrigin();
        ray.direction_ = WorldRayDirection();
    
        float3 unit_aabb[2];
        unit_aabb[0] = float3(0.0f, 0.0f, 0.0f);
        unit_aabb[1] = float3(1.f, 1.f, 1.f);
    
        float t_min, t_max;
        if (RayAABBIntersectionTest(ray, unit_aabb, t_min, t_max))
        {
            // Perform sphere tracing through the AABB.
            uint i = 0;
            while (i++ < MAX_SPHERE_TRACING_STEPS && t_min <= t_max)
            {
                float3 position = ray.origin_ + max(t_min, 0) * ray.direction_;
                
                float distance = GetDistance(position);

                // Has the ray intersected the primitive? 
                if (distance <= SPHERE_TRACING_THRESHOLD)
                {
                    RayIntersectionAttributes attributes;
                    
                    attributes.normal_ = CalculateNormal(position);
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

    if (constant_buffer_.rendering_flags_ & RENDERING_FLAG_VISUALIZE_AABBS)
    {
        payload.colour_ = float4(1, 0.0, 0.0, 1);
        return;
    }
    else if (constant_buffer_.rendering_flags_ & RENDERING_FLAG_VISUALIZE_PARTICLES)
    {
        payload.colour_ = float4(0.306, 0.941, 0.933, 1);
        return;
    }
    else if (constant_buffer_.rendering_flags_ & RENDERING_FLAG_NORMALS)
    {
        payload.colour_ = float4(attributes.normal_, 1.f);
        return;
    }
    else
    {
        float3 position = WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
        float4 specular = float4(0.9f, 0.9f, 0.9f, 1);
        payload.colour_ = float4(0.306, 0.941, 0.933, 1) * CalculateLighting(attributes.normal_, constant_buffer_.camera_pos_, position, specular) + specular;
    }
}


// -------------- Miss Shader ----------------
[shader("miss")]
void MissShader(inout RayPayload payload)
{
    payload.colour_ = float4(0.0f, 0.2f, 0.4f, 1.0f);
}


#endif // RAYTRACING_HLSL