cbuffer MatrixBuffer : register(b0, space0)
{
    matrix viewMatrix;
    matrix projectionMatrix;
};

struct InstanceData
{
    float4x4 WorldMatrix;
    float2 TexcoordURange;
    float2 TexcoordVRange;
};

StructuredBuffer<InstanceData> Instances : register(t0, space0);

struct InputType
{
    float3 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 tex : TEXCOORD;
};

struct OutputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
    float3 worldPosition : POSITION1;
};

OutputType main(InputType input, uint InstanceID : SV_InstanceID)
{
    OutputType output;
    
    float4x4 worldMatrix = Instances[InstanceID].WorldMatrix;

	// Calculate the position of the vertex against the world, view, and projection matrices.
    output.position = mul(float4(input.position, 1), worldMatrix);
    output.position = mul(output.position, viewMatrix);
    output.position = mul(output.position, projectionMatrix);

    // Resolve sub-UVs
    output.tex.x = lerp(Instances[InstanceID].TexcoordURange.x,
        Instances[InstanceID].TexcoordURange.y,
        input.tex.x);
    output.tex.y = lerp(Instances[InstanceID].TexcoordVRange.x,
        Instances[InstanceID].TexcoordURange.y,
        input.tex.y);
    
	// Calculate the normal vector against the world matrix only and normalise.
    output.normal = mul(input.normal, (float3x3) worldMatrix);
    output.normal = normalize(output.normal);
	
    float4 worldPosHomo = mul(float4(input.position, 1), worldMatrix);
    
    output.worldPosition = worldPosHomo.xyz / worldPosHomo.w;

    return output;
}