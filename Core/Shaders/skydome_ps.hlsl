struct VSOutput
{
    float4 position : SV_Position;
    float3 texcoord : TEXCOORD0;
};

cbuffer SunBuffer : register(b0)
{
    float3 g_sunColour;
    float g_sunExp;
    float3 g_lightdirection;
};

float4 main(VSOutput input) : SV_TARGET
{
    float3 up = float3(0, 1, 0);
    
    float cosTheta = dot(normalize(input.texcoord), up);
    
    float3 orange = float3(0.86, 0.49, 0.06);
    float3 blue = float3(0.15, 0.19, 0.84);
    float3 black = float3(0, 0, 0);
    
    float3 skyColour;
    
    if (cosTheta < 0)
    {          
        float lerpFactor = saturate(-5 * cosTheta);
        skyColour = lerp(orange, black, lerpFactor);
    }
    else
    {
        skyColour = lerp(orange, blue, cosTheta);
    }
    
    float3 L = -g_lightdirection;
    float3 V = normalize(input.texcoord);
    float LdotV = max(dot(L, V), 0);
    
    float3 sunColour = (g_sunExp / 16) * g_sunColour * pow(LdotV, g_sunExp);
    
    return float4(sunColour + skyColour, 1);
}