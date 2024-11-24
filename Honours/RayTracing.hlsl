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
    if (constant_buffer_.rendering_flags_ & RENDERING_FLAG_VISUALIZE_PARTICLES)
    {
        if (RenderParticlesVisualized())
        {
            RayIntersectionAttributes attributes;
            ReportHit(1, 0, attributes);            
        }
    }
    else
    {
        Ray ray;
        ray.origin_ = WorldRayOrigin();
        ray.direction_ = WorldRayDirection();
    
        float3 aabb[2];
        aabb[0] = float3(0.0f, 0.0f, 0.0f);
        aabb[1] = float3(1.f, 1.f, 1.f);
    
        float t_min, t_max;
        if (RayAABBIntersectionTest(ray, aabb, t_min, t_max))
        {
            // Perform sphere tracing through the AABB.
            uint i = 0;
            while (i++ < MAX_SPHERE_TRACING_STEPS && t_min <= t_max)
            {
                float3 position = ray.origin_ + max(t_min, 0) * ray.direction_;
                
                //float distance = GetAnalyticalSignedDistance(position);
                float distance = sdf_texture_.SampleLevel(sampler_, position, 0);
                
                // Has the ray intersected the primitive? 
                if (distance <= MAX_SPHERE_TRACING_THRESHOLD)
                {
                    RayIntersectionAttributes attributes;
                    ReportHit(max(t_min, RayTMin()), 0, attributes);
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

    payload.colour_ = (float4(0.306, 0.941, 0.933, 1) * (length(WorldRayOrigin()) - RayTCurrent() + 0.6));

}


// -------------- Miss Shader ----------------
[shader("miss")]
void MissShader(inout RayPayload payload)
{
    payload.colour_ = float4(0.0f, 0.2f, 0.4f, 1.0f);
}


#endif // RAYTRACING_HLSL