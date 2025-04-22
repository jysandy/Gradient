#include "ShadowMapping.hlsli"
#include "LightStructs.hlsli"
#include "PBRLighting.hlsli"

SamplerState linearSampler : register(s0, space3);
SamplerComparisonState shadowMapSampler : register(s1, space3);

Texture2D shadowMap : register(t1, space3);
TextureCube environmentMap : register(t2, space3);
TextureCubeArray pointShadowMaps : register(t3, space3);

#define MAX_POINT_LIGHTS 8

cbuffer LightBuffer : register(b0, space3)
{
    DirectionalLight g_directionalLight;
    PointLight g_pointLights[MAX_POINT_LIGHTS];
    uint g_numPointLights;
};

cbuffer Constants : register(b1, space3)
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

float4 Water_PS(InputType input) : SV_TARGET
{
    float3 N = normalize(input.normal);
    
    float3 V = normalize(cameraPosition - input.worldPosition);
    
    float ao = 1.f;
    float metalness = 1.f;
    float roughness = 0.2f;


    // The colour transmitted by shallow water
    float3 shallowWaterColour = float3(0.14, 0.94, 0.94) * 0.5;
    // The colour transmitted by deep water
    float3 deepWaterColour = float3(0.06, 0.11, 0.19) * 0.5;
    // Water is more reflective at grazing view angles and more transmissive at 
    // oblique angles.
    float reflectanceFactor = pow(1.f - pow(max(dot(N, V), 0), 2.5), 5);
    
    float3 albedo = lerp(deepWaterColour, float3(1, 1, 1), reflectanceFactor);
    
    // Using the height here as a proxy for thickness.
    // The peaks of each wave are thinner than 
    // the troughs.    
    float heightRatio = saturate(input.worldPosition.y / maxAmplitude);
    float thickness = 1 - pow(heightRatio, thicknessPower);
    
    float3 directRadiance = DirectionalLightContribution(
        N, V, albedo, metalness, roughness, g_directionalLight,
        shadowMap, shadowMapSampler, shadowTransform, input.worldPosition
    );

    float3 directSSS = directionalLightSSS(g_directionalLight,
                                           N,
                                           V,
                                           thickness,
                                           sharpness,
                                           refractiveIndex) * shallowWaterColour;
    
    float3 pointRadiance = float3(0, 0, 0);
    float3 pointSSS = float3(0, 0, 0);
    for (int i = 0; i < g_numPointLights; i++)
    {
        pointRadiance += PointLightContribution(
            N, V, albedo, metalness, roughness, g_pointLights[i],
            pointShadowMaps, shadowMapSampler, input.worldPosition
        );
        
        pointSSS += pointLightSSS(g_pointLights[i],
                                  input.worldPosition,
                                  N,
                                  V,
                                  thickness,
                                  sharpness,
                                  refractiveIndex) * shallowWaterColour;
    }
    
    float3 ambient = cookTorranceEnvironmentMap(
        environmentMap, linearSampler,
        N, V, albedo, ao, metalness, roughness
    );
    
    float3 outputColour = ambient 
        + directRadiance
        + pointRadiance
        + directSSS
        + pointSSS;

    //return float4(1, 1, 1, 1);    
    return float4(outputColour, 1.f);
}