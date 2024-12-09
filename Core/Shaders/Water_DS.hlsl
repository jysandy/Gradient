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

cbuffer LodBuffer : register(b2)
{
    float3 g_cameraPosition;
    float g_minLodDistance;
    float3 g_cameraDirection;
    float g_maxLodDistance;
    uint  g_cullingEnabled;
    float g_pad1[3];
}

// Interpolate normals with the up vector 
// as distance increases to reduce noise.
float3 lodNormal(float3 N, float3 worldP)
{
    const float d0 = g_minLodDistance; 
    const float d1 = g_maxLodDistance;
    
    float d = distance(worldP, g_cameraPosition);
    float s = 1 - saturate((d - d0) / (d1 - d0));
    
    return lerp(float3(0, 1, 0), N, pow(s, 2.5));
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

    interpolatedLocalPosition.y = 0;    
    float3 originalWorldP = mul(float4(interpolatedLocalPosition, 1.f), worldMatrix).xyz;
    
    for (int i = 0; i < g_numWaves; i++)            
    {
        interpolatedLocalPosition.y += waveHeight(interpolatedLocalPosition,
                                                    g_waves[i],
                                                    g_totalTime);
        dx += ddxWaveHeight(interpolatedLocalPosition, g_waves[i], g_totalTime);
        dz += ddzWaveHeight(interpolatedLocalPosition, g_waves[i], g_totalTime);
    }
       
    Output.normal = normalize(float3(-dx, 1, -dz));
    
    Output.normal = lodNormal(Output.normal, originalWorldP);
	
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
