#ifndef SDF_HELPERS_HLSL
#define SDF_HELPERS_HLSL

#ifdef RAYTRACING_HLSL
#include "RayTracingCommon.hlsli"
#endif
#ifdef COMPUTE_TEX_HLSL
StructuredBuffer<ParticlePosition> particle_positions_ : register(t0);
#endif

#define LIGHT_DIRECTION -1.f, -1.f, 1.f
#define SPECULAR_POWER 50.f
#define AMBIENT 0.5f

// quadratic polynomial smooth minimum
// (Evans, 2015, pp. 30) https://advances.realtimerendering.com/s2015/AlexEvans_SIGGRAPH-2015-sml.pdf 
// r is the radius of influence
float SmoothMin(float a, float b, float r)
{
    float e = max(r - abs(a - b), 0.0f);
    return min(a, b) - e * e * 0.25f / r;
}

float GetDistanceToSphere(float3 displacement, float radius)
{
    return length(displacement) - radius;
}

float GetAnalyticalSignedDistance(float3 position)
{
    // vector between the particle position and the current sphere tracing position
    
    float distance = GetDistanceToSphere(particle_positions_[0].position_ - position, 0.1f);
    for (int x = 1; x < NUM_PARTICLES; x++)
    {
        float distance1 = GetDistanceToSphere(particle_positions_[x].position_ - position, 0.1f);      
        distance = SmoothMin(distance, distance1, 0.05);
        //distance = min(distance, distance1);
    }
    
    return distance;
}

#ifndef COMPUTE_TEX_HLSL

float GetDistance(float3 position)
{
    if (constant_buffer_.rendering_flags_ & RENDERING_FLAG_ANALYTICAL)
    {
        // Find distance analytically
        return GetAnalyticalSignedDistance(position);
    }
    else
    {
        // Sample the distance from the SDF texture
        return sdf_texture_.SampleLevel(sampler_, position, 0);
    }
}

// https://iquilezles.org/articles/normalsSDF/
float3 CalculateNormal(float3 position)
{
    float step;
    if (constant_buffer_.rendering_flags_ & RENDERING_FLAG_ANALYTICAL)
    {
        step = 0.001f;
    }
    else
    {
        step = constant_buffer_.uvw_step_;
    }
    
    const float2 h = float2(step, 0);
    return normalize(float3(GetDistance(position + h.xyy) - GetDistance(position - h.xyy),
                            GetDistance(position + h.yxy) - GetDistance(position - h.yxy),
                            GetDistance(position + h.yyx) - GetDistance(position - h.yyx)));
}

float CalculateLighting(float3 normal, float3 cameraPos, float3 worldPosition, inout float4 specular)
{
    float3 lightVector = -float3(LIGHT_DIRECTION);
    
    float diffuse = saturate(dot(normal, lightVector));
    
    // Blinn-phong specular calculation
    float3 viewVector = normalize(cameraPos - worldPosition);
    float3 halfway = normalize(lightVector + viewVector);
    specular *= pow(max(dot(normal, halfway), 0.0), SPECULAR_POWER);
    
    return diffuse + AMBIENT;
}

#endif

#endif