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

    float3 centrePosition = float3(0, 0, 0);
	
    float d = distance(interpolatedLocalPosition, centrePosition);

    float insideTerm = d * d;
	
    float3 tangent =
    {
        1,
		cos(d - g_totalTime) * (interpolatedLocalPosition.x - centrePosition.x) / d,
		0
    };
	
    float3 bitangent =
    {
        0,
		cos(d - g_totalTime) * (interpolatedLocalPosition.z - centrePosition.z) / d,
		1
    };

    float3 c = cross(bitangent, tangent);
    if (dot(c, c) == 0)
    {
        Output.normal = float3(0, 1, 0);
    }
    else
    {
        Output.normal = normalize(c);
    }
    interpolatedLocalPosition.y = sin(d - g_totalTime);
	
    Output.vPosition = mul(float4(interpolatedLocalPosition, 1.f), worldMatrix);
    Output.worldPosition = Output.vPosition.xyz;
    Output.vPosition = mul(Output.vPosition, viewMatrix);
    Output.vPosition = mul(Output.vPosition, projectionMatrix);
			
    Output.tex = patch[0].tex * domain.x
		+ patch[1].tex * domain.y
		+ patch[2].tex * domain.z;
	
	return Output;
}
