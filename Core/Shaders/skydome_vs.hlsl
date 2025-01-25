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

VSOutput main(float4 pos : SV_Position)
{
    VSOutput vout;

    vout.position = mul(mul(mul(pos, worldMatrix),
                            viewMatrix),
                        projectionMatrix);
    vout.position.z = vout.position.w; // Draw on far plane
    vout.texcoord = pos.xyz;

    return vout;
}