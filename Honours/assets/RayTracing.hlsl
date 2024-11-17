#ifndef RAYTRACING_HLSL
#define RAYTRACING_HLSL

#define MAX_RECURSION_DEPTH 1 // Primary rays

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
    float example;
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
    RayIntersectionAttributes attributes;
    ReportHit(1, 0, attributes);
}


// -------------- Closest Hit Shader ----------------

[shader("closesthit")]
void ClosestHitShader(inout RayPayload payload, in RayIntersectionAttributes attributes)
{
    payload.colour_ = float4(0.537f, 0.984f, 1, 1);
}


// -------------- Miss Shader ----------------
[shader("miss")]
void MissShader(inout RayPayload payload)
{
    payload.colour_ = float4(0.0f, 0.2f, 0.4f, 1.0f);
}


#endif // RAYTRACING_HLSL