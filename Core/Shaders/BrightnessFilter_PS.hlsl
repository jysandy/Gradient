Texture2D<float4> Texture : register(t0);
sampler TextureSampler : register(s0);

cbuffer BloomParams : register(b0, space1)
{
    float g_exposure;
    float g_intensity;
};

float4 main(float4 color : COLOR0,
    float2 texCoord : TEXCOORD0) : SV_Target0
{
    float4 outputColor = Texture.Sample(TextureSampler, texCoord);
    float3 luminanceFactors = float3(0.2126, 0.7152, 0.0722);
    
    float luminance = dot(luminanceFactors, outputColor.rgb);
    
    float luminanceFilter = step((20 - g_exposure), luminance);
    
    return float4(outputColor.rgb * luminanceFilter * g_intensity, outputColor.a);    
}