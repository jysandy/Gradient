struct InputType
{
    float3 position : LOCALPOS;
    float3 normal : NORMAL;
    float2 tex : TEXCOORD;
};

InputType main(InputType input)
{
    return input;
}