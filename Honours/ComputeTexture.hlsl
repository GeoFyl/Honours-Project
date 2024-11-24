#ifndef COMPUTE_TEX_HLSL
#define COMPUTE_TEX_HLSL

#include "ComputeCommon.hlsli"
#include "SdfHelpers.hlsli"

#define WORLD_MIN float3(0,0,0)
#define WORLD_MAX float3(1,1,1)

//StructuredBuffer<ParticlePosition> particle_positions_ : register(t0);
RWTexture3D<snorm float> output_texture_ : register(u0);

// Shader for creating SDF 3D texture 
[numthreads(32, 32, 1)]
void CSTexMain(int3 dispatch_ID : SV_DispatchThreadID)
{
    float tex_res = TEXTURE_RESOLUTION - 1.0f;
    float3 position = lerp(WORLD_MIN, WORLD_MAX, dispatch_ID / float3(tex_res, tex_res, tex_res));
    output_texture_[dispatch_ID] = GetAnalyticalSignedDistance(position);
}

#endif