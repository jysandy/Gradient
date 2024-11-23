struct Wave
{
    float amplitude;
    float wavelength;
    float speed;
    float sharpness;
    float3 direction;
};

cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
    float g_totalTime;
};

#define MAX_WAVES 32
            
cbuffer WaveBuffer : register(b1)
{
    uint g_numWaves;
    float3 g_pad;
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


float waveHeight(float3 position, 
    Wave wave,
    float time)
{
    float w = 2.f / max(wave.wavelength, 0.0001);
    float phi = wave.speed * 2.f / max(wave.wavelength, 0.0001);
    
    float sinTerm = pow((sin(dot(wave.direction, position) * w - time * phi) + 1) / 2.f,
                        wave.sharpness);
    return 2 * wave.amplitude * sinTerm;
}

float ddxWaveHeight(float3 position,
    Wave wave,
    float time)
{
    float w = 2.f / max(wave.wavelength, 0.0001);
    float phi = wave.speed * 2.f / max(wave.wavelength, 0.0001);

    float DoP = dot(wave.direction, position);
    
    float sinTerm = pow((sin(DoP * w - time * phi) + 1) / 2.f,
                        wave.sharpness - 1);
    float cosTerm = cos(DoP * w - time * phi);
    return wave.sharpness * wave.direction.x * w * wave.amplitude * sinTerm * cosTerm;
}

float ddzWaveHeight(float3 position,
    Wave wave,
    float time)
{
    float w = 2.f / max(wave.wavelength, 0.00001);
    float phi = wave.speed * 2.f / max(wave.wavelength, 0.00001);

    float DoP = dot(wave.direction, position);
    
    float sinTerm = pow((sin(DoP * w - time * phi) + 1) / 2.f,
                        wave.sharpness - 1);
    float cosTerm = cos(DoP * w - time * phi);
    return wave.sharpness * wave.direction.z * w * wave.amplitude * sinTerm * cosTerm;
}

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
