Texture2D<float4> Texture : register(t0);
sampler TextureSampler : register(s0);

// The ACES tonemapper.
// Adapted from https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
float3 ACESFilm(float3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

float4 main(float4 color : COLOR0,
    float2 texCoord : TEXCOORD0) : SV_Target0
{
    float4 outputColor;
    
    outputColor = Texture.Sample(TextureSampler, texCoord);
    
    outputColor.rgb = ACESFilm(outputColor.rgb);
    outputColor.a = 1;
    
    return outputColor;
}