Texture2D<float4> Texture : register(t0);
sampler TextureSampler : register(s0);

float4 GaussianHorizontal_PS(float4 color : COLOR0,
    float2 texCoord : TEXCOORD0) : SV_Target0
{
    float width, height;
    Texture.GetDimensions(width, height);
    
    float du = 1 / width;
    float dv = 1 / height;
    
    float2 kernelCoords[5] =
    {
        { -2 * du, 0 },
        { -du, 0 },
        { 0, 0 },
        { du, 0 },
        { 2 * du, 0 },
    };
    
    float kernel[5] =
    {
        6, 24, 36, 24, 6
    };
    
    float4 outputColor = { 0, 0, 0, 0 };
    
    [unroll]
    for (int i = 0; i < 5; ++i)
    {
        outputColor += kernel[i] * Texture.Sample(TextureSampler,
            texCoord + kernelCoords[i]);
    }
    outputColor.rgb /= 96;
    
    outputColor.a = 1;
    
    return outputColor;
}