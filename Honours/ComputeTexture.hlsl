#ifndef COMPUTE_TEX_HLSL
#define COMPUTE_TEX_HLSL

#include "ComputeCommon.hlsli"
#include "SdfHelpers.hlsli"

RWTexture3D<snorm float> output_texture_ : register(u0);

// Shader for creating the simple SDF 3D texture 
[numthreads(32, 32, 1)]
void CSTexMain(int3 dispatch_ID : SV_DispatchThreadID)
{
    float3 position = lerp(WORLD_MIN, WORLD_MAX, ((dispatch_ID + 0.5f) / TEXTURE_RESOLUTION));
    output_texture_[dispatch_ID] = GetAnalyticalSignedDistance(position);
}

#endif