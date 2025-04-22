Texture2D<float4> Texture : register(t0);
sampler TextureSampler : register(s0);

float4 GaussianVertical_PS(float4 color : COLOR0,
    float2 texCoord : TEXCOORD0) : SV_Target0
{
    float width, height;
    Texture.GetDimensions(width, height);
    
    float dv = 1 / height;
    
    float2 kernelCoords[5] =
    {
        { 0, -2 * dv },
        { 0, -dv },
        { 0, 0 },
        { 0, dv },
        { 0, 2 * dv },
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