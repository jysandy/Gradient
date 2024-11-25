#include "ShadowMapping.hlsli"
#include "LightStructs.hlsli"
#include "PBRLighting.hlsli"

SamplerState linearSampler : register(s0);
SamplerComparisonState shadowMapSampler : register(s1);

Texture2D shadowMap : register(t1);
TextureCube environmentMap : register(t2);

cbuffer LightBuffer : register(b0)
{
    DirectionalLight directionalLight;
};

cbuffer Constants : register(b1)
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
    float3 N = normalize(input.normal);
    
    float3 V = normalize(cameraPosition - input.worldPosition);
    
    float3 albedo = float3(1, 1, 1);
    float ao = 1.f;
    
    float metalness = 1.f;
    float roughness = 0.2f;
    
    float3 directRadiance = cookTorranceDirectionalLight(
        N, V, albedo, metalness, roughness, directionalLight
    );
    
    float3 ambient = cookTorranceEnvironmentMap(
        environmentMap, linearSampler,
        N, V, albedo, ao, metalness, roughness
    );
    
    float shadowFactor = calculateShadowFactor(
        shadowMap,
        shadowMapSampler,
        shadowTransform,
        input.worldPosition);
    
    float3 directIrradiance = directionalLightIrradiance(directionalLight);
    float3 L = -normalize(directionalLight.direction);
    float heightRatio = saturate(input.worldPosition.y / maxAmplitude);
    
    // Using the height here as a proxy for thickness.
    // The peaks of each wave are thinner than 
    // the troughs.
    float3 sss = subsurfaceScattering(directIrradiance,
                                      N, 
                                      V, 
                                      L, 
                                      1 - heightRatio, 
                                      0.5);
    
    float3 outputColour = ambient 
        + shadowFactor * directRadiance
        + sss;

    //return float4(1, 1, 1, 1);    
    return float4(outputColour, 1.f);
}