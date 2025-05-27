#include "NormalMapping.hlsli"
#include "ShadowMapping.hlsli"
#include "LightStructs.hlsli"
#include "PBRLighting.hlsli"
#include "Utils.hlsli"

// Shared with the vertex shader
Texture2D heightmap : register(t0, space0);
SamplerState linearSampler : register(s0, space0);

Texture2D albedoMap : register(t0, space2);
Texture2D shadowMap : register(t1, space2);
Texture2D normalMap : register(t2, space2);
Texture2D aoMap : register(t3, space2);
Texture2D metallicMap : register(t4, space2);
Texture2D roughnessMap : register(t5, space2);
TextureCube environmentMap : register(t6, space2);
TextureCubeArray pointShadowMaps : register(t7, space2);
Texture2D<uint> ssaoMap : register(t8, space2);

SamplerState anisotropicSampler : register(s0, space2);
SamplerComparisonState shadowMapSampler : register(s1, space2);

#define MAX_POINT_LIGHTS 8

cbuffer LightBuffer : register(b0, space2)
{
    DirectionalLight g_directionalLight;
    PointLight g_pointLights[MAX_POINT_LIGHTS];
    uint g_numPointLights;
};

cbuffer Constants : register(b1, space2)
{
    float3 cameraPosition;
    float uvTiling;
    float4x4 shadowTransform; // TODO: put this into the light
}

struct InputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
    float3 worldPosition : POSITION1;
};

float4 Heightmap_PS(InputType input) : SV_TARGET
{
    //return float4(1, 1, 1, 1);
    
    input.tex *= uvTiling;
    
    // The heightmap PS doesn't suppport masking
    float4 albedoSample = albedoMap.Sample(anisotropicSampler, input.tex);
    
    input.normal = normalize(input.normal);
    
    float3 N = perturbNormal(
        normalMap,
        linearSampler,
        input.normal,
        input.worldPosition,
        input.tex);
    
    float3 V = normalize(cameraPosition - input.worldPosition);
    
    float3 albedo = albedoSample.rgb;
    uint screenWidth, screenHeight;
    ssaoMap.GetDimensions(screenWidth, screenHeight);
    uint ssao = ssaoMap.Sample(linearSampler, float2(input.position.x / screenWidth, input.position.y / screenHeight));
    float ao = min(aoMap.Sample(linearSampler, input.tex).r, (ssao / 255.f));
    float metalness = metallicMap.Sample(linearSampler, input.tex).r;
    float roughness = roughnessMap.Sample(linearSampler, input.tex).r;

    float3 directRadiance = DirectionalLightContribution(
        N, V, albedo, metalness, roughness, g_directionalLight,
        shadowMap, shadowMapSampler, shadowTransform, input.worldPosition
    );
    
    float3 pointRadiance = float3(0, 0, 0);
    for (int i = 0; i < g_numPointLights; i++)
    {
        pointRadiance += PointLightContribution(
            N, V, albedo, metalness, roughness, g_pointLights[i],
            pointShadowMaps, shadowMapSampler, input.worldPosition
        );
    }
    
    float3 ambient = cookTorranceEnvironmentMap(
        environmentMap, linearSampler,
        N, V, albedo, ao, metalness, roughness
    );
    
    float3 outputColour = ambient
        + directRadiance
        + pointRadiance;

    outputColour = ApplyFog(outputColour, input.worldPosition, cameraPosition);
    
    //return float4(1, 1, 1, 1);    
    return float4(outputColour, 1.f);
}