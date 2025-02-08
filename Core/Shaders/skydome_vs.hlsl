struct InputType
{
    float3 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 tex : TEXCOORD;
};

struct VSOutput
{
    float4 position : SV_Position;
    float3 texcoord : TEXCOORD0;
};

cbuffer MatrixBuffer : register(b0, space0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
};

VSOutput main(InputType input)
{
    VSOutput vout;

    vout.position = mul(mul(mul((float4(input.position, 1)), worldMatrix),
                            viewMatrix),
                        projectionMatrix);
    vout.position.z = vout.position.w; // Draw on far plane
    vout.texcoord = input.position.xyz;

    return vout;
}