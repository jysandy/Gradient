#include "NormalMapping.hlsli"
#include "ShadowMapping.hlsli"
#include "LightStructs.hlsli"
#include "PBRLighting.hlsli"

// Shared with the vertex shader
Texture2D heightmap : register(t0, space0);
SamplerState linearSampler : register(s0, space0);

Texture2D shadowMap : register(t1, space2);
TextureCube environmentMap : register(t6, space2);
TextureCubeArray pointShadowMaps : register(t7, space2);

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
    float maxAmplitude;
    float4x4 shadowTransform; // TODO: put this into the light
}

struct InputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
    float3 worldPosition : POSITION1;
};

float4 main(InputType input) : SV_TARGET
{
    //return float4(1, 1, 1, 1);
    input.normal = normalize(input.normal);
    
    float3 N = input.normal;
    
    float3 V = normalize(cameraPosition - input.worldPosition);
    
    float3 albedo = float3(0.3, 0.3, 0.3);
    float ao = 1.f;
    float metalness = 0.f;
    float roughness = 0.8f;

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

    //return float4(1, 1, 1, 1);    
    return float4(outputColour, 1.f);
}