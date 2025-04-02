#ifndef RAYTRACING_HELPERS_HLSL
#define RAYTRACING_HELPERS_HLSL

#include "RayTracingCommon.hlsli"

// From Microsoft samples
// Generate a ray in world space for a camera pixel corresponding to an index from the dispatched 2D grid.
inline void GenerateCameraRay(uint2 dispatch_index, out float3 ray_origin, out float3 ray_direction)
{
    float2 xy = dispatch_index + 0.5f; // center in the middle of the pixel.
    float2 screenPos = xy / DispatchRaysDimensions().xy * 2.0 - 1.0;

    // Invert Y for DirectX-style coordinates.
    screenPos.y = -screenPos.y;

    // Unproject the pixel coordinate into a ray.
    float4 world = mul(float4(screenPos, 0, 1), rt_constant_buffer_.inv_view_proj_);

    world.xyz /= world.w;
    ray_origin = rt_constant_buffer_.camera_pos_;
    ray_direction = normalize(world.xyz - ray_origin);
}

// Traces a primary ray
RayPayload TracePrimaryRay(in Ray ray, in uint current_recursion_depth)
{
    RayPayload payload;
    payload.colour_ = float4(0.0f, 0.0f, 0.0f, 0.0f);

    if (current_recursion_depth >= MAX_RECURSION_DEPTH)
	{
		return payload;
	}

    // Set the ray's extents.
	RayDesc rayDesc;
	rayDesc.Origin = ray.origin_;
	rayDesc.Direction = ray.direction_;
	rayDesc.TMin = 0.01f; // Todo: see how changing this affects things (0.0f?)
	rayDesc.TMax = 10000;

	// Todo: will have to update this if add more types of rays
	TraceRay(scene_, RAY_FLAG_NONE, ~0, 0, 1, 0, rayDesc, payload);

	return payload;
}

// Test if a ray segment <RayTMin(), RayTCurrent()> intersects an AABB.
// Limitation: this test does not take RayFlags into consideration and does not calculate a surface normal.
// Ref: https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-box-intersection
bool RayAABBIntersectionTest(Ray ray, float3 aabb[2], out float tmin, out float tmax)
{
    float3 tmin3, tmax3;
    int3 sign3 = ray.direction_ > 0;

    // Handle rays parallel to any x|y|z slabs of the AABB.
    // If a ray is within the parallel slabs, 
    //  the tmin, tmax will get set to -inf and +inf
    //  which will get ignored on tmin/tmax = max/min.
    // If a ray is outside the parallel slabs, -inf/+inf will
    //  make tmax > tmin fail (i.e. no intersection).
    // TODO: handle cases where ray origin is within a slab 
    //  that a ray direction is parallel to. In that case
    //  0 * INF => NaN
    const float FLT_INFINITY = 1.#INF;
    float3 invRayDirection = select(ray.direction_ != 0, 1 / ray.direction_, select(ray.direction_ > 0, FLT_INFINITY, -FLT_INFINITY));

    tmin3.x = (aabb[1 - sign3.x].x - ray.origin_.x) * invRayDirection.x;
    tmax3.x = (aabb[sign3.x].x - ray.origin_.x) * invRayDirection.x;

    tmin3.y = (aabb[1 - sign3.y].y - ray.origin_.y) * invRayDirection.y;
    tmax3.y = (aabb[sign3.y].y - ray.origin_.y) * invRayDirection.y;
    
    tmin3.z = (aabb[1 - sign3.z].z - ray.origin_.z) * invRayDirection.z;
    tmax3.z = (aabb[sign3.z].z - ray.origin_.z) * invRayDirection.z;
    
    tmin = max(max(tmin3.x, tmin3.y), tmin3.z);
    tmax = min(min(tmax3.x, tmax3.y), tmax3.z);
    
    return tmax > tmin && tmax >= RayTMin() && tmin <= RayTCurrent();
}

bool RenderParticlesVisualized()
{
    float aspect_ratio = (float)DispatchRaysDimensions().x / (float)DispatchRaysDimensions().y;    
    float2 xy = DispatchRaysIndex().xy + 0.5f; // center in the middle of the pixel.
    float2 screenPos = xy / DispatchRaysDimensions().xy * 2.0 - 1.0;

    // Invert Y for DirectX-style coordinates and adjust for aspect ratio
    screenPos.y = -screenPos.y;
    screenPos.x *= aspect_ratio;    

    [unroll]
    for (int x = 0; x < NUM_PARTICLES; x++)
    {
        float3 particle_pos = particles_[x].position_;
        float3 cam_pos = rt_constant_buffer_.camera_pos_;
        
        if (dot(particle_pos - cam_pos, rt_constant_buffer_.camera_lookat_ - cam_pos) > 0)
        {
            // Project particle from world to screen space and adjust for aspect ratio
            float4 particleScreenpos = mul(float4(particle_pos, 1), rt_constant_buffer_.view_proj_);
            particleScreenpos.xyz /= particleScreenpos.w;
            particleScreenpos.x *= aspect_ratio;
        
            // Report hit in small radius around particle (draws circle)
            if (length(particleScreenpos.xy - screenPos) < 0.015)
            {
                return true;
            }            
        }
    }
    return false;
}



#endif