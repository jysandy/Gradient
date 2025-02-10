Texture2D heightmap : register(t0, space0);

SamplerState linearSampler : register(s0, space0);

struct InputType
{
    float3 position : LOCALPOS;
    float3 normal : NORMAL;
    float2 tex : TEXCOORD;
};

InputType main(InputType input)
{
    float displacement = heightmap.SampleLevel(linearSampler, input.tex, 0).r
        * 50.f;
    
    input.position.y += displacement;
    
    return input;
}