#ifndef __UTILS_HLSLI__
#define __UTILS_HLSLI__

float InverseLerp(float value, float a, float b)
{
    return saturate((value - a) / (b - a));
}

float3 ApplyFog(float3 colour, float3 worldPosition, float3 cameraPosition)
{
    float cameraDistance = distance(worldPosition, cameraPosition);
    float fogAlpha = InverseLerp(cameraDistance, 50, 300);
    float heightAlpha = InverseLerp(worldPosition.y, 50, 0);

    float finalAlpha = fogAlpha * heightAlpha;
    
    return lerp(colour, 2 * float3(0.8, 0.8, 0.8), lerp(0, 0.3, pow(finalAlpha, 3)));
}

#endif