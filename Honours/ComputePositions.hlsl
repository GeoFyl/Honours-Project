#ifndef COMPUTE_POS_HLSL
#define COMPUTE_POS_HLSL

#include "ComputeCommon.hlsli"

RWStructuredBuffer<ParticleData> particle_positions_ : register(u0);
ConstantBuffer<ComputeCB> constant_buffer_ : register(b1);

// From https://stackoverflow.com/a/10625698
float random(float2 p)
{
    float2 K1 = float2(
        23.14069263277926, // e^pi (Gelfond's constant)
         2.665144142690225 // 2^sqrt(2) (Gelfond–Schneider constant)
    );
    return frac(cos(dot(p, K1)) * 12345.6789);
}

// Gets the coords within a grid of variable size given an index
float3 IndexTo3DCoords(uint index, uint per_axis)
{
    float3 coords = float3(0, 0, 0);
    
    uint per_z = per_axis * per_axis;
    coords.z = index / per_z;
    coords.y = (index % per_z) / per_axis;
    coords.x = index % per_axis;
    
    return coords;
}

// Shader for generating particles
[numthreads(1024, 1, 1)]
void CSParticleGen(int3 dispatch_ID : SV_DispatchThreadID)
{
    if (dispatch_ID.x >= NUM_PARTICLES)
    {
        //ID overshoots end of buffer - nothing to do here.
        return;
    }
    
    ParticleData particle = (ParticleData)0;    
    
    if (SCENE == SceneRandom) // Generate particles randomly
    {
        float2 seed = float2(dispatch_ID.x, dispatch_ID.x);
    
        particle.position_.x = random(seed);
        seed.x += particle.position_.x;
        
        particle.position_.y = random(seed);
        seed.y += particle.position_.y;   
        
        particle.position_.z = random(seed);
        seed.x += particle.position_.z;
        
        particle.start_pos_ = particle.position_;
        
        particle.speed_ = random(seed) * 4;

    }
    else if (SCENE == SceneGrid) // Place particles in a grid, tightly packed so each particle is touching its neighbours
    {
        int particles_per_axis = max(pow(NUM_PARTICLES, 1.0 / 3.0), 1);

        float3 lower = float3(PARTICLE_RADIUS + 0.01f, PARTICLE_RADIUS + 0.01f, PARTICLE_RADIUS + 0.01f);
        
        float up = min((max(particles_per_axis - 1, 1) * PARTICLE_RADIUS * 2) + PARTICLE_RADIUS, 0.99f - PARTICLE_RADIUS);
        float3 upper = float3(up, up, up);
        
        lower += (0.99f - PARTICLE_RADIUS - up) / 2; //centre everything
        upper += (0.99f - PARTICLE_RADIUS - up) / 2;
        
        float3 particle_coords = lerp(lower, upper, IndexTo3DCoords(dispatch_ID.x, particles_per_axis) / max(particles_per_axis - 1, 1));
        
        particle.position_ = particle_coords;
        particle.start_pos_ = particle.position_;
        particle.speed_ = 0;
    }
    else if (SCENE == SceneWave) // Same as above but squish the height-wise and make some of the particles faster than the rest
    {
        int particles_per_axis = pow(NUM_PARTICLES, 1.0 / 3.0);

        float3 lower = float3(PARTICLE_RADIUS + 0.01f, PARTICLE_RADIUS + 0.01f, PARTICLE_RADIUS + 0.01f);

        float up = min(((particles_per_axis - 1) * PARTICLE_RADIUS * 2) + PARTICLE_RADIUS, 0.99f - PARTICLE_RADIUS);
        float3 upper = float3(up, 0.3, up);
       
        lower.xz += (0.99f - PARTICLE_RADIUS - up) / 2; //centre everything
        upper.xz += (0.99f - PARTICLE_RADIUS - up) / 2;
        
        float3 particle_coords = lerp(lower, upper, IndexTo3DCoords(dispatch_ID.x, particles_per_axis) / (particles_per_axis - 1));
        
        particle.position_ = particle_coords;
        particle.start_pos_ = particle.position_;
        
        particle.speed_ = 1;        
        if (random(float2(dispatch_ID.x, particle.position_.z)) > 0.95) // 5% chance to be speedy
        {
            particle.speed_ = 3;
        }
    }
    else if (SCENE == SceneNormals) // The first 10 particles are put in a sine wave shape, the rest are put in the corner
    {
        if (dispatch_ID.x > 9)
        {
            particle.position_ = float3(0.1, 0.1, 0.1);
        }
        else
        {
            float upper = min((10 - 1) * PARTICLE_RADIUS * 2, 0.99f - PARTICLE_RADIUS);
            particle.position_ = float3(upper * ((float) dispatch_ID.x / 10) + PARTICLE_RADIUS + 0.01f, 0.1 * sin(5 * (float) dispatch_ID.x / 10) + 0.5, 0.5f);            
        }

        particle.start_pos_ = particle.position_;
        particle.speed_ = 0;
    }
    
    particle_positions_[dispatch_ID.x] = particle;
    
    return;
}

// Shader for manipulating particle positions
[numthreads(1024, 1, 1)]
void CSPosMain(int3 dispatch_ID : SV_DispatchThreadID)
{
    if (dispatch_ID.x >= NUM_PARTICLES)
    {
        //ID overshoots end of buffer - nothing to do here.
        return;
    }
    
    ParticleData particle = particle_positions_[dispatch_ID.x];
    
    if (SCENE == SceneRandom) // Animate particles by moving them back and forth along a random vector
    {
        float3 direction = normalize(float3(random(float2(dispatch_ID.x, particle.speed_)) - 0.5, random(float2(dispatch_ID.x, particle.start_pos_.z)) - 0.5, random(float2(dispatch_ID.x, particle.start_pos_.y)) - 0.5));
        particle.position_ = particle.start_pos_ + direction * 0.2 * sin(particle.speed_ * constant_buffer_.time_);
    }
    else if (SCENE == SceneWave) // Animate particles by moving them up and down in a sine wave according to the x position
    {
        particle.position_.y = particle.start_pos_.y + 0.1 * sin(5 * particle.position_.x + 2 * constant_buffer_.time_ * particle.speed_);
    }
    
    // Clamp to within the bounds
    particle.position_ = clamp(particle.position_, WORLD_MIN + 0.1f, WORLD_MAX - 0.1f);
    
    // Store position
    particle_positions_[dispatch_ID.x].position_ = particle.position_;
    
    return;
}

#endif