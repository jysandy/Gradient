struct VS_CONTROL_POINT_OUTPUT
{
	float3 vPosition : LOCALPOS;
    float3 normal : NORMAL;
    float2 tex : TEXCOORD;
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

cbuffer Constants : register(b0)
{
    matrix g_worldMatrix;
    float3 g_cameraPosition;
}

#define NUM_CONTROL_POINTS 3

HS_CONSTANT_DATA_OUTPUT CalcHSPatchConstants(
	InputPatch<VS_CONTROL_POINT_OUTPUT, NUM_CONTROL_POINTS> ip,
	uint PatchID : SV_PrimitiveID)
{
	HS_CONSTANT_DATA_OUTPUT Output;

    float3 centreL = (ip[0].vPosition + ip[1].vPosition + ip[2].vPosition) / 3.f;
    float3 centreW = mul(float4(centreL, 1.f), g_worldMatrix).xyz;
    float d = distance(centreW, g_cameraPosition);
	
    const float d0 = 2.f;   // Highest LOD
    const float d1 = 50.f;  // Lowest LOD
    const float minTess = 1;
    const float maxTess = 3;
	
    float s = saturate((d - d0) / (d1 - d0));
    float tess = pow(2, lerp(maxTess, minTess, s));
	
    Output.EdgeTessFactor[0] =
		Output.EdgeTessFactor[1] =
		Output.EdgeTessFactor[2] = 
		Output.InsideTessFactor = tess;

	return Output;
}

[domain("tri")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("CalcHSPatchConstants")]
HS_CONTROL_POINT_OUTPUT main( 
	InputPatch<VS_CONTROL_POINT_OUTPUT, NUM_CONTROL_POINTS> ip, 
	uint i : SV_OutputControlPointID,
	uint PatchID : SV_PrimitiveID )
{
	HS_CONTROL_POINT_OUTPUT Output;

	Output.vPosition = ip[i].vPosition;
    Output.normal = ip[i].normal;
    Output.tex = ip[i].tex;

	return Output;
}
