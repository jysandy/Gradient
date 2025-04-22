#include "WaterWaves.hlsli"

#define MAX_WAVES 32

cbuffer WaveBuffer : register(b0, space0)
{
    uint g_numWaves;
    float g_totalTime;
    float2 g_pad;
    Wave g_waves[MAX_WAVES];
}

struct InputType
{
    float3 position : LOCALPOS;
    float3 normal : NORMAL;
    float2 tex : TEXCOORD;
};

InputType Water_VS(InputType input)
{
    uint numWaves = min(MAX_WAVES, g_numWaves);
    for (int i = 0; i < g_numWaves; i++)
    {
        input.position.y += waveHeight(input.position,
                                       g_waves[i],
                                       g_totalTime);
    }
    
    return input;
}