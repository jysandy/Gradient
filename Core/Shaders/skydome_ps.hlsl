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
    float g_sunIrradiance;
    float g_ambientIrradiance;
};

float4 main(VSOutput input) : SV_TARGET
{
    float3 up = float3(0, 1, 0);
    float3 L = -normalize(g_lightdirection);
    float3 V = normalize(input.texcoord);
    
    float3 horizontalL = normalize(float3(L.x, lerp(L.y, 0, g_drawSunCircle), L.z));
    
    float cosTheta = dot(normalize(input.texcoord), up);
    float cosL = max(dot(L, up), 0);
    float cosHorizontalL = dot(horizontalL, V) * 0.5 + 0.5;
    
    float3 blue = float3(0.1, 0.14, 0.6);
    float3 black = float3(0, 0, 0);
    
    float3 skyColour;
    float3 skyBase = lerp(g_sunColour, blue, cosL);
    skyBase = lerp(blue, skyBase, cosHorizontalL);

    // TODO: Get the sun flare up at the height of the sun
    skyColour = g_ambientIrradiance * lerp(skyBase, blue, pow(abs(cosTheta), lerp(0.8, 1, g_drawSunCircle)));

    float LdotV = max(dot(L, V), 0);
    
    float3 sunCircle = g_drawSunCircle * g_sunIrradiance * g_sunColour * pow(LdotV, g_sunExp);
    
    return float4(sunCircle + skyColour, 1);
}