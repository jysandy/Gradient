#ifndef __LIGHT_STRUCTS_HLSLI__
#define __LIGHT_STRUCTS_HLSLI__

struct DirectionalLight
{
    float3 colour;
    float irradiance;
    float3 direction;
    float pad;
};

struct PointLight
{
    float3 colour;
    float irradiance;
    float3 position;
    float maxRange;
};

#endif