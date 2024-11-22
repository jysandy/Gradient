#ifndef __LIGHT_STRUCTS_HLSLI__
#define __LIGHT_STRUCTS_HLSLI__

struct DirectionalLight
{
    float3 colour;
    float irradiance;
    float3 direction;
};

#endif