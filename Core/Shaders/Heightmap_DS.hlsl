// Shared with the vertex shader
Texture2D heightmap : register(t0, space0);
SamplerState linearSampler : register(s0, space0);

// Shared with the hull shader
cbuffer MatrixBuffer : register(b0, space1)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
};

// Shared with the hull shader
cbuffer LodBuffer : register(b1, space1)
{
    float3 g_cameraPosition;
    float g_minLodDistance;
    float3 g_cameraDirection;
    float g_maxLodDistance;
    uint g_cullingEnabled;
    float g_pad1[3];
}

// Shared with the vertex shader
cbuffer HeightMapParamsBuffer : register(b2, space1)
{
    float g_hmHeight;
    float g_hmGridWidth;
}

struct DS_OUTPUT
{
    float4 vPosition : SV_POSITION;
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
    float EdgeTessFactor[3] : SV_TessFactor;
    float InsideTessFactor : SV_InsideTessFactor;
};

#define NUM_CONTROL_POINTS 3

[domain("tri")]
DS_OUTPUT Heightmap_DS(
	HS_CONSTANT_DATA_OUTPUT input,
	float3 domain : SV_DomainLocation,
	const OutputPatch<HS_CONTROL_POINT_OUTPUT, NUM_CONTROL_POINTS> patch)
{
    DS_OUTPUT Output;                                            
    Output.normal = float3(0, 1, 0);
    
    float3 interpolatedLocalPosition = patch[0].vPosition * domain.x
		+ patch[1].vPosition * domain.y
		+ patch[2].vPosition * domain.z;
    
    Output.tex = patch[0].tex * domain.x
		+ patch[1].tex * domain.y
		+ patch[2].tex * domain.z;
    
    float height = g_hmHeight;
    float gridWidth = g_hmGridWidth;
    
    float displacement = heightmap.SampleLevel(linearSampler, Output.tex, 0).r
        * height;
    interpolatedLocalPosition.y = displacement;
    
    float du = 0.01;
    float dx = du * gridWidth;
    
    // +x 
    float3 fooPosition = interpolatedLocalPosition;
    fooPosition.x += dx;
    fooPosition.y = heightmap.SampleLevel(linearSampler, 
            Output.tex + float2(du, 0), 
            0).r
        * height;
    float3 tangent = fooPosition - interpolatedLocalPosition;
    
    //+z
    fooPosition = interpolatedLocalPosition;
    fooPosition.z += dx;
    fooPosition.y = heightmap.SampleLevel(linearSampler,
            Output.tex + float2(0, du),
            0).r
        * height;
    float3 bitangent = fooPosition - interpolatedLocalPosition;
    
    float3 normal = cross(bitangent, tangent);

    if(isnan(normal.y))
        Output.normal = float3(0, 1, 0);
    else
        Output.normal = normalize(normal);
    
    // DEBUG: REMOVEME
    //interpolatedLocalPosition.y = 0;

    Output.vPosition = mul(float4(interpolatedLocalPosition, 1.f), worldMatrix);
    Output.worldPosition = Output.vPosition.xyz / Output.vPosition.w;
    Output.vPosition = mul(Output.vPosition, viewMatrix);
    Output.vPosition = mul(Output.vPosition, projectionMatrix);
    Output.normal = mul(Output.normal, (float3x3) worldMatrix);
    return Output;
}
