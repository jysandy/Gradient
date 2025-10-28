cbuffer Material : register(b2, space1)
{
    uint g_albedoMapIndex;
    uint g_normalMapIndex;
    uint g_metallicMapIndex;
    uint g_roughnessMapIndex;
    uint g_aoMapIndex;
}

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
float4 MaskedDepth_PS(InputType input) : SV_Target
{
    Texture2D albedoMap = ResourceDescriptorHeap[g_albedoMapIndex];
    float4 albedoSample = albedoMap.Sample(anisotropicSampler, input.tex);
    clip(albedoSample.a - 0.01);

    return float4(0, 0, 0, albedoSample.a);
}