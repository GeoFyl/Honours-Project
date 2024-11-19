#ifndef RAYTRACING_HLSL
#define RAYTRACING_HLSL

#define MAX_RECURSION_DEPTH 1 // Primary rays
#define MAX_SPHERE_TRACING_STEPS 512
#define MAX_SPHERE_TRACING_THRESHOLD 0.001f

struct Ray
{
    float3 origin_;
    float3 direction_;
};

struct RayPayload
{
    float4 colour_;
};

struct RayIntersectionAttributes
{
    float distance_;
};

struct RayTracingCB
{
    float4x4 inv_view_proj_;
    float3 camera_pos_;
};

RaytracingAccelerationStructure scene_ : register(t0);
RWTexture2D<float4> render_target_ : register(u0);
ConstantBuffer<RayTracingCB> constant_buffer_ : register(b0);

// From Microsoft samples
// Generate a ray in world space for a camera pixel corresponding to an index from the dispatched 2D grid.
inline void GenerateCameraRay(uint2 dispatch_index, out float3 ray_origin, out float3 ray_direction)
{
    float2 xy = dispatch_index + 0.5f; // center in the middle of the pixel.
    float2 screenPos = xy / DispatchRaysDimensions().xy * 2.0 - 1.0;

    // Invert Y for DirectX-style coordinates.
    screenPos.y = -screenPos.y;

    // Unproject the pixel coordinate into a ray.
    float4 world = mul(float4(screenPos, 0, 1), constant_buffer_.inv_view_proj_);

    world.xyz /= world.w;
    ray_origin = constant_buffer_.camera_pos_;
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

// quadratic polynomial smooth minimum
// https://iquilezles.org/articles/smin/
float SmoothMin(float a, float b, float k)
{
    k *= 4.0;
    float h = max(k - abs(a - b), 0.0) / k;
    return min(a, b) - h * h * k * (1.0 / 4.0);
}

float GetDistanceToSphere(float3 displacement, float radius)
{
    return length(displacement) - radius;
}

float GetAnalyticalSignedDistance(float3 position)
{
    // vector between the particle position and the current sphere tracing position
    // particle postions currently hard coded
    float3 particle_positions[25];
    int index = 0;
    
    
    for (int i = 0; i < 5; i++)
    {
        for (int j = 0; j < 5; j++)
        {
            particle_positions[index] = float3((j + 5) * 0.1, (i + 5) * 0.1, 0.3);
            index++;
        }
    }
    
    float distance = GetDistanceToSphere(particle_positions[0] - position, 0.1f);
    for (int x = 1; x < 25; x++)
    {
        float distance1 = GetDistanceToSphere(particle_positions[x] - position, 0.1f);            
        distance = SmoothMin(distance, distance1, 0.05f);
        //distance = min(distance, distance1);
    }
    
    return distance;
    
    // radius hard coded as 0.3
    //return SmoothMin(distance0, distance1, 0.05f);
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