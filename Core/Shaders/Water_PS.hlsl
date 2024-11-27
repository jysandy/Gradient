#include "ShadowMapping.hlsli"
#include "LightStructs.hlsli"
#include "PBRLighting.hlsli"

SamplerState linearSampler : register(s0);
SamplerComparisonState shadowMapSampler : register(s1);

Texture2D shadowMap : register(t1);
TextureCube environmentMap : register(t2);
TextureCubeArray pointShadowMaps : register(t3);

#define MAX_POINT_LIGHTS 8

cbuffer LightBuffer : register(b0)
{
    DirectionalLight directionalLight;
    PointLight g_pointLights[MAX_POINT_LIGHTS];
    uint g_numPointLights;
};

cbuffer Constants : register(b1)
{
    float3 cameraPosition;
    float maxAmplitude;
    float4x4 shadowTransform; // TODO: put this into the light
    float thicknessPower;
    float sharpness;
    float refractiveIndex;
    float pad;
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
    float3 N = normalize(input.normal);
    
    float3 V = normalize(cameraPosition - input.worldPosition);
    
    float3 albedo = float3(1, 1, 1);
    float ao = 1.f;
    
    float metalness = 1.f;
    float roughness = 0.2f;

    // Using the height here as a proxy for thickness.
    // The peaks of each wave are thinner than 
    // the troughs.    
    float heightRatio = saturate(input.worldPosition.y / maxAmplitude);
    float thickness = 1 - pow(heightRatio, thicknessPower);
    
    float3 directRadiance = cookTorranceDirectionalLight(
        N, V, albedo, metalness, roughness, directionalLight
    );

    float3 directSSS = directionalLightSSS(directionalLight,
                                           N,
                                           V,
                                           thickness,
                                           sharpness,
                                           refractiveIndex);
    
    float3 pointRadiance = float3(0, 0, 0);
    float3 pointSSS = float3(0, 0, 0);
    for (int i = 0; i < g_numPointLights; i++)
    {
        pointRadiance += cookTorrancePointLight(
            N, V, albedo, metalness, roughness, g_pointLights[i], input.worldPosition
        )
        * cubeShadowFactor(pointShadowMaps,
            shadowMapSampler,
            g_pointLights[i].position,
            input.worldPosition,
            g_pointLights[i].shadowTransforms,
            g_pointLights[i].shadowCubeIndex);
        
        pointSSS += pointLightSSS(g_pointLights[i],
                                  input.worldPosition,
                                  N,
                                  V,
                                  thickness,
                                  sharpness,
                                  refractiveIndex);
    }
    
    float3 ambient = cookTorranceEnvironmentMap(
        environmentMap, linearSampler,
        N, V, albedo, ao, metalness, roughness
    );
    
    float shadowFactor = calculateShadowFactor(
        shadowMap,
        shadowMapSampler,
        shadowTransform,
        input.worldPosition);
    
    float3 outputColour = ambient 
        + shadowFactor * directRadiance
        + pointRadiance
        + directSSS
        + pointSSS;

    //return float4(1, 1, 1, 1);    
    return float4(outputColour, 1.f);
}