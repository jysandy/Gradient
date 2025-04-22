Texture2D heightmap : register(t0, space0);

SamplerState linearSampler : register(s0, space0);

cbuffer HeightMapParamsBuffer : register(b2, space1)
{
    float g_hmHeight;
    float g_hmGridWidth;
}

struct InputType
{
    float3 position : LOCALPOS;
    float3 normal : NORMAL;
    float2 tex : TEXCOORD;
};

InputType Heightmap_VS(InputType input)
{
    float displacement = heightmap.SampleLevel(linearSampler, input.tex, 0).r
        * g_hmHeight;
    
    input.position.y += displacement;
    
    return input;
}