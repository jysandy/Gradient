cbuffer MatrixBuffer : register(b0, space0)
{
    matrix worldMatrix;
    matrix worldViewProjectionMatrix;
};

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

OutputType main(InputType input)
{
    OutputType output;

    output.position = mul(float4(input.position, 1), worldViewProjectionMatrix);

    output.tex = input.tex;

    output.normal = mul(input.normal, (float3x3) worldMatrix);
    output.normal = normalize(output.normal);
	
    float4 worldPosHomo = mul(float4(input.position, 1), worldMatrix);
    
    output.worldPosition = worldPosHomo.xyz / worldPosHomo.w;

    return output;
}