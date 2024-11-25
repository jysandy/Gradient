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
    float pad;
    float3 g_cameraDirection;
}

float3 toWorld(float3 local)
{
    return mul(float4(local, 1.f), g_worldMatrix).xyz;
}

float cameraDot(float3 worldP)
{
    return dot(normalize(worldP - g_cameraPosition), g_cameraDirection);
}

float3 tessFactor(float3 worldP)
{    
    const float d0 = 50.f; // Highest LOD
    const float d1 = 400.f; // Lowest LOD
    const float minTess = 1;
    const float maxTess = 6;
		
    float d = distance(worldP, g_cameraPosition);
    float s = saturate((d - d0) / (d1 - d0));

    return pow(2, lerp(maxTess, minTess, s));
}

#define NUM_CONTROL_POINTS 3

// TODO: This is breaking in release mode 
// when optimizations are enabled.
HS_CONSTANT_DATA_OUTPUT CalcHSPatchConstants(
	InputPatch<VS_CONTROL_POINT_OUTPUT, NUM_CONTROL_POINTS> ip,
	uint PatchID : SV_PrimitiveID)
{
	HS_CONSTANT_DATA_OUTPUT Output;

    float3 e0 = toWorld(0.5 * (ip[0].vPosition + ip[1].vPosition));
    float3 e1 = toWorld(0.5 * (ip[1].vPosition + ip[2].vPosition));
    float3 e2 = toWorld(0.5 * (ip[2].vPosition + ip[0].vPosition));
    float3 c = toWorld((ip[0].vPosition + ip[1].vPosition + ip[2].vPosition) / 3.f);
	
    Output.EdgeTessFactor[0] = tessFactor(e1);
    Output.EdgeTessFactor[1] = tessFactor(e2);
    Output.EdgeTessFactor[2] = tessFactor(e0);
    Output.InsideTessFactor = tessFactor(c);

    float maxCameraDot = max(cameraDot(e0),
                   max(cameraDot(e1),
                   max(cameraDot(e2),
                       cameraDot(c))));
    
    // Cull patches that are behind the camera.
    if (maxCameraDot < -0.1)
    {
        Output.EdgeTessFactor[0] = 
        Output.EdgeTessFactor[1] = 
        Output.EdgeTessFactor[2] = 
        Output.InsideTessFactor = 0;
    }
    
	return Output;
}

[domain("tri")]
[partitioning("fractional_even")]
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
