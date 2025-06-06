#ifndef RAYTRACING_STRUCTS_HLSL
#define RAYTRACING_STRUCTS_HLSL

#include "ComputeCommon.hlsli"

#define MAX_RECURSION_DEPTH 1 // Primary rays
#define MAX_SPHERE_TRACING_STEPS 512
#define SPHERE_TRACING_THRESHOLD 0.001f

// Rendering flags
#define RENDERING_FLAG_NONE                     0
#define RENDERING_FLAG_VISUALIZE_PARTICLES      1 << 0
#define RENDERING_FLAG_ANALYTICAL               1 << 1
#define RENDERING_FLAG_NORMALS                  1 << 2
#define RENDERING_FLAG_VISUALIZE_AABBS          1 << 3
#define RENDERING_FLAG_SIMPLE_AABB              1 << 4

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
    float3 float_3_; // Can be used for normals or brick AABB uvw 
};

struct RayTracingCB
{
    float4x4 view_proj_;
    float4x4 inv_view_proj_;
    float3 camera_pos_;
    float padding_;
    float3 camera_lookat_;    
    uint rendering_flags_;
};


// Global arguments
RaytracingAccelerationStructure scene_ : register(t0);
StructuredBuffer<ParticleData> particles_ : register(t1);
Texture3D<snorm float> sdf_texture_ : register(t2);
StructuredBuffer<AABB> AABBs_ : register(t3);
RWTexture2D<float4> render_target_ : register(u0);
ConstantBuffer<RayTracingCB> rt_constant_buffer_ : register(b1);
ConstantBuffer<ComputeCB> comp_constant_buffer_ : register(b2);
SamplerState linear_sampler_ : register(s0);


#endif