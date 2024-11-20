#ifndef SDF_HELPERS_HLSL
#define SDF_HELPERS_HLSL


// quadratic polynomial smooth minimum
// (Evans, 2015, pp. 30) https://advances.realtimerendering.com/s2015/AlexEvans_SIGGRAPH-2015-sml.pdf 
float SmoothMin(float a, float b, float k)
{
    float e = max(k - abs(a - b), 0.0f);
    return min(a, b) - e * e * 0.25f / k;
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
        distance = SmoothMin(distance, distance1, 0.05);
        //distance = min(distance, distance1);
    }
    
    return distance;
}

#endif