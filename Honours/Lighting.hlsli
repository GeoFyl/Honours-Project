#ifndef RAYTRACING_STRUCTS_HLSL
#define RAYTRACING_STRUCTS_HLSL

#define MAX_RECURSION_DEPTH 1 // Primary rays
#define MAX_SPHERE_TRACING_STEPS 512
#define MAX_SPHERE_TRACING_THRESHOLD 0.01f

#define NUM_PARTICLES 125 // Also in ComputeStructs.h, ComputeCommon.hlsli
#define TEXTURE_RESOLUTION 256 // Also in ComputeCommon.hlsli

// Rendering flags
#define RENDERING_FLAG_NONE 0
#define RENDERING_FLAG_VISUALIZE_PARTICLES 1 << 0
#define RENDERING_FLAG_ANALYTICAL 1 << 1

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
    float3 normal_;
};

struct RayTracingCB
{
    float4x4 view_proj_;
    float4x4 inv_view_proj_;
    float3 camera_pos_;
    uint rendering_flags_;
};

struct ParticlePosition
{
    float3 position_;
    float speed;
    float start_y;
};

RaytracingAccelerationStructure scene_ : register(t0);
StructuredBuffer<ParticlePosition> particle_positions_ : register(t1);
Texture3D<snorm float> sdf_texture_ : register(t2);
RWTexture2D<float4> render_target_ : register(u0);
ConstantBuffer<RayTracingCB> constant_buffer_ : register(b0);
SamplerState sampler_ : register(s0);

#endif