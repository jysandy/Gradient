#include "NormalMapping.hlsli"
#include "ShadowMapping.hlsli"
#include "LightStructs.hlsli"
#include "PBRLighting.hlsli"

Texture2D albedoMap : register(t0);
SamplerState anisotropicSampler : register(s0);

Texture2D shadowMap : register(t1);
SamplerComparisonState shadowMapSampler : register(s1);

Texture2D normalMap : register(t2);
SamplerState pointSampler : register(s2);

Texture2D aoMap : register(t3);
SamplerState linearSampler : register(s3);

Texture2D metalnessMap : register(t4);
Texture2D roughnessMap : register(t5);

TextureCube environmentMap : register(t6);

#define MAX_POINT_LIGHTS 8

cbuffer LightBuffer : register(b0)
{
    DirectionalLight g_directionalLight;
    PointLight g_pointLights[MAX_POINT_LIGHTS];
    uint g_numPointLights;
};

cbuffer Constants : register(b1)
{
    float3 cameraPosition;
    float pad;
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
    input.normal = normalize(input.normal);
    
    float3 N = perturbNormal(
        normalMap,
        linearSampler,
        input.normal,
        input.worldPosition,
        input.tex);
    float3 V = normalize(cameraPosition - input.worldPosition);
    
    float4 albedoSample = albedoMap.Sample(anisotropicSampler, input.tex);
    float3 albedo = albedoSample.rgb;
    float ao = aoMap.Sample(linearSampler, input.tex).r;
    float metalness = metalnessMap.Sample(linearSampler, input.tex).r;
    float roughness = roughnessMap.Sample(linearSampler, input.tex).r;
    
    float3 directRadiance = cookTorranceDirectionalLight(
        N, V, albedo, metalness, roughness, g_directionalLight
    );
    
    float3 pointRadiance = float3(0, 0, 0);
    for (int i = 0; i < g_numPointLights; i++)
    {
        pointRadiance += cookTorrancePointLight(
            N, V, albedo, metalness, roughness, g_pointLights[i], input.worldPosition
        );
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
        + pointRadiance;
    
    return float4(outputColour, albedoSample.a);
}