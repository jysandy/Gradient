Texture2D albedoMap : register(t0, space1);
SamplerState anisotropicSampler : register(s0, space1);

struct InputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
    float3 worldPosition : POSITION1;
};

// This shader only exists to clip masked pixels 
// when drawing shadows.
void MaskedDepth_PS(InputType input)
{
    float4 albedoSample = albedoMap.Sample(anisotropicSampler, input.tex);
    clip(albedoSample.a - 0.01);
}