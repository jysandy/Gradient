Texture2D<float4> Texture : register(t0);
Texture2D<float4> TextureToBlend : register(t0, space1);
sampler TextureSampler : register(s0);

float4 AdditiveBlend_PS(float4 color : COLOR0,
    float2 texCoord : TEXCOORD0) : SV_Target0
{
    float4 outputColor
        = Texture.Sample(TextureSampler, texCoord)
        + TextureToBlend.Sample(TextureSampler, texCoord);

    outputColor.a = saturate(outputColor.a);
    return outputColor;
}