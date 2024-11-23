cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
    float g_totalTime;
};

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
    float amplitude, 
    float wavelength, 
    float speed,
    float k,
    float3 direction,
    float time)
{
    float w = 2.f / wavelength;
    float phi = speed * 2.f / wavelength;
    
    float sinTerm = pow((sin(dot(direction, position) * w - time * phi) + 1) / 2.f,
                        k);
    return 2 * amplitude * sinTerm;
}

float ddxWaveHeight(float3 position,
    float amplitude,
    float wavelength,
    float speed,
    float k,
    float3 direction,
    float time)
{
    float w = 2.f / wavelength;
    float phi = speed * 2.f / wavelength;

    float DoP = dot(direction, position);
    
    float sinTerm = pow((sin(DoP * w - time * phi) + 1) / 2.f,
                        k - 1);
    float cosTerm = cos(DoP * w - time * phi);
    return k * direction.x * w * amplitude * sinTerm * cosTerm;
}

float ddzWaveHeight(float3 position,
    float amplitude,
    float wavelength,
    float speed,
    float k,
    float3 direction,
    float time)
{
    float w = 2.f / wavelength;
    float phi = speed * 2.f / wavelength;

    float DoP = dot(direction, position);
    
    float sinTerm = pow((sin(DoP * w - time * phi) + 1) / 2.f,
                        k - 1);
    float cosTerm = cos(DoP * w - time * phi);
    return k * direction.z * w * amplitude * sinTerm * cosTerm;
}

float3 waveNormal(float3 position,
    float amplitude,
    float wavelength,
    float speed,
    float k,
    float3 direction,
    float time)
{
    float dx = ddxWaveHeight(
                    position,
                    amplitude,
                    wavelength,
                    speed,
                    k,
                    direction,
                    time);
    float dz = ddzWaveHeight(
                    position,
                    amplitude,
                    wavelength,
                    speed,
                    k,
                    direction,
                    time);

    float3 N = float3(-dx, 1, -dz);
    float NoN = dot(N, N);
    
    
    if (NoN == 0)
        return float3(0, 1, 0);
    
    return normalize(N);
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

    float amplitude = 0.2;
    float wavelength = 2.f;
    float speed = 0.5;
    float k = 2;
    float3 direction = normalize(float3(-1, 0, 0.8));
    
    Output.normal = waveNormal(interpolatedLocalPosition,
                                amplitude,
                                wavelength,
                                speed,
                                k,
                                direction,
                                g_totalTime);
    
    interpolatedLocalPosition.y = waveHeight(
                                    interpolatedLocalPosition,
                                    amplitude,
                                    wavelength,
                                    speed,
                                    k,
                                    direction,
                                    g_totalTime);
	
    Output.vPosition = mul(float4(interpolatedLocalPosition, 1.f), worldMatrix);
    Output.worldPosition = Output.vPosition.xyz;
    Output.vPosition = mul(Output.vPosition, viewMatrix);
    Output.vPosition = mul(Output.vPosition, projectionMatrix);
			
    Output.tex = patch[0].tex * domain.x
		+ patch[1].tex * domain.y
		+ patch[2].tex * domain.z;
	
	return Output;
}
