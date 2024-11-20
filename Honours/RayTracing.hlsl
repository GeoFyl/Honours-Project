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
            float3 position = ray.origin_ + t_min * ray.direction_;
            float distance = GetAnalyticalSignedDistance(position);

            // Has the ray intersected the primitive? 
            if (distance <= MAX_SPHERE_TRACING_THRESHOLD)
            {
                //float3 hitSurfaceNormal = sdCalculateNormal(position, sdPrimitive);
                RayIntersectionAttributes attributes;
                ReportHit(max(t_min, RayTMin()), 0, attributes);
            }

            // Since distance is the minimum distance to the primitive, 
            // we can safely jump by that amount without intersecting the primitive.
            t_min += distance;
        }
    }
    
    //float3 particle_positions[25];
    //int index = 0;
    
    //for (int i = 0; i < 5; i++)
    //{
    //    for (int j = 0; j < 5; j++)
    //    {
    //        particle_positions[index] = float3((j + 5) * 0.1, (i + 5) * 0.1, 0.3);
    //        index++;
    //    }
    //}
    
    //[unroll]
    //for (int x = 0; x < 25; x++)
    //{
    //    float2 xy = DispatchRaysIndex().xy + 0.5f; // center in the middle of the pixel.
    //    float2 screenPos = xy / DispatchRaysDimensions().xy * 2.0 - 1.0;

    //    // Invert Y for DirectX-style coordinates.
    //    screenPos.y = -screenPos.y;
        
    //    float4 particleScreenpos = mul(float4(particle_positions[x], 1), constant_buffer_.view_proj_);
    //    particleScreenpos.xyz /= particleScreenpos.w;
        
    //    if (length(particleScreenpos.xy - screenPos) < 0.01)
    //    {
    //        RayIntersectionAttributes attributes;
    //        ReportHit(1, 0, attributes);
    //    }

    //}

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