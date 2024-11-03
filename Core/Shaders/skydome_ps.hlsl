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
    float g_drawSunCircle;
};

float4 main(VSOutput input) : SV_TARGET
{
    float3 up = float3(0, 1, 0);
    float3 L = -normalize(g_lightdirection);
    float3 V = normalize(input.texcoord);
    
    float3 horizontalL = normalize(float3(L.x, 0, L.z));
    
    float cosTheta = dot(normalize(input.texcoord), up);
    float cosL = max(dot(L, up), 0);
    float cosHorizontalL = dot(horizontalL, V) * 0.5 + 0.5;
    
    float3 orange = float3(0.86, 0.49, 0.06);
    float3 blue = float3(0.1, 0.14, 0.6);
    float3 black = float3(0, 0, 0);
    
    float3 skyColour;
    float3 skyBase = lerp(orange, blue, cosL);
    skyBase = lerp(blue, skyBase, cosHorizontalL);
    if (cosTheta < 0)
    {          
        float lerpFactor = saturate(-2 * cosTheta);
        skyColour = lerp(skyBase, black, lerpFactor);
    }
    else
    {
        skyColour = lerp(skyBase, blue, cosTheta);
    }
    
    
    
    float LdotV = max(dot(L, V), 0);
    
    float3 sunCircle = g_drawSunCircle * (g_sunExp / 16) * g_sunColour * pow(LdotV, g_sunExp);
    
    return float4(sunCircle + skyColour, 1);
}