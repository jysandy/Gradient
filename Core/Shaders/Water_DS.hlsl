#include "WaterWaves.hlsli"

cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
};

#define MAX_WAVES 32
            
cbuffer WaveBuffer : register(b1)
{
    uint g_numWaves;
    float g_totalTime;
    float2 g_pad;
    Wave g_waves[MAX_WAVES];
}

struct DS_OUTPUT
{
	float4 vPosition  : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
    float3 worldPosition : POSITION1;
};

struct HS_CONTROL_POINT_OUTPUT
{
    float3 vPosition : LOCALPOS;
    float3 normal : NORMAL;
    float2 tex : TEXCOORD;
};


struct HS_CONSTANT_DATA_OUTPUT
{
	float EdgeTessFactor[3]			: SV_TessFactor;
	float InsideTessFactor			: SV_InsideTessFactor;
};

#define NUM_CONTROL_POINTS 3

[domain("tri")]
DS_OUTPUT main(
	HS_CONSTANT_DATA_OUTPUT input,
	float3 domain : SV_DomainLocation,
	const OutputPatch<HS_CONTROL_POINT_OUTPUT, NUM_CONTROL_POINTS> patch)
{
    DS_OUTPUT Output;
    
    float3 interpolatedLocalPosition = patch[0].vPosition * domain.x
		+ patch[1].vPosition * domain.y
		+ patch[2].vPosition * domain.z;
    
    float dx = 0;
    float dz = 0;
    
    uint numWaves = min(MAX_WAVES, g_numWaves);
    interpolatedLocalPosition.y = 0;
    for (int i = 0; i < g_numWaves; i++)            
    {
        interpolatedLocalPosition.y += waveHeight(interpolatedLocalPosition,
                                                    g_waves[i],
                                                    g_totalTime);
        dx += ddxWaveHeight(interpolatedLocalPosition, g_waves[i], g_totalTime);
        dz += ddzWaveHeight(interpolatedLocalPosition, g_waves[i], g_totalTime);
    }
       
    Output.normal = normalize(float3(-dx, 1, -dz));
    
    if (isnan(Output.normal.y))
        Output.normal = float3(0, 1, 0);
	
    Output.vPosition = mul(float4(interpolatedLocalPosition, 1.f), worldMatrix);
    Output.worldPosition = Output.vPosition.xyz;
    Output.vPosition = mul(Output.vPosition, viewMatrix);
    Output.vPosition = mul(Output.vPosition, projectionMatrix);
			
    Output.tex = patch[0].tex * domain.x
		+ patch[1].tex * domain.y
		+ patch[2].tex * domain.z;
	
	return Output;
}