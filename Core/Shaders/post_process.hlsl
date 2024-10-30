Texture2D<float4> Texture : register(t0);
sampler TextureSampler : register(s0);

float4 main(float4 color : COLOR0,
    float2 texCoord : TEXCOORD0) : SV_Target0
{
    float width, height;
    Texture.GetDimensions(width, height);
    
    float du = 1 / width;
    float dv = 1 / height;
    
    float2 kernelCoords[9] =
    {
        { -du, -dv },
        {0, -dv },
        {du, -dv },
        
        {-du, 0 },
        {0, 0 },
        {du, 0 },
        
        {-du, dv},
        {0, dv },
        {du, dv }
    };
    
    float kernel[9] =
    {
        0, -1, 0,
        -1, 5, -1,
        0, -1, 0
    };
    
    float4 outputColor = { 0, 0, 0, 0 };
    
    [unroll]
    for (int i = 0; i < 9; ++i)
    {
        outputColor += kernel[i] * Texture.Sample(TextureSampler, 
            texCoord + kernelCoords[i]);
    }
            
    outputColor.a = 1;
    
    return outputColor;
}