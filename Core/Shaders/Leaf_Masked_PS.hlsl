#include "NormalMapping.hlsli"
#include "ShadowMapping.hlsli"
#include "LightStructs.hlsli"
#include "PBRLighting.hlsli"
#include "Utils.hlsli"

Texture2D shadowMap : register(t1, space1);
TextureCube environmentMap : register(t6, space1);
TextureCubeArray pointShadowMaps : register(t7, space1);

SamplerState linearSampler : register(s3, space1);
SamplerState anisotropicSampler : register(s0, space1);
SamplerComparisonState shadowMapSampler : register(s1, space1);

#define MAX_POINT_LIGHTS 8

cbuffer LightBuffer : register(b0, space1)
{
    DirectionalLight g_directionalLight;
    PointLight g_pointLights[MAX_POINT_LIGHTS];
    uint g_numPointLights;
};

cbuffer Constants : register(b1, space1)
{
    float3 cameraPosition;
    float uvTiling;
    float3 emissiveRadiance;
    uint g_ssaoMapIndex;
    float4x4 shadowTransform; // TODO: put this into the light
};

cbuffer Material : register(b2, space1)
{
    uint g_albedoMapIndex;
    uint g_normalMapIndex;
    uint g_metallicMapIndex;
    uint g_roughnessMapIndex;
    uint g_aoMapIndex;
};

struct InputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
    float3 worldPosition : POSITION1;
};

float4 Leaf_Masked_PS(InputType input) : SV_TARGET
{
    input.tex *= uvTiling;

    // resolve bindless textures
    Texture2D albedoMap = ResourceDescriptorHeap[g_albedoMapIndex];
    Texture2D normalMap = ResourceDescriptorHeap[g_normalMapIndex];
    Texture2D metalnessMap = ResourceDescriptorHeap[g_metallicMapIndex];
    Texture2D roughnessMap = ResourceDescriptorHeap[g_roughnessMapIndex];
    Texture2D aoMap = ResourceDescriptorHeap[g_aoMapIndex];

    float4 albedoSample = albedoMap.Sample(anisotropicSampler, input.tex);
    // This shader supports masking
    clip(albedoSample.a - 0.01);

    input.normal = normalize(input.normal);

    float3 N = perturbNormal(
        normalMap,
        linearSampler,
        input.normal,
        input.worldPosition,
        input.tex);
    float3 V = normalize(cameraPosition - input.worldPosition);


    float3 albedo = albedoSample.rgb;

    Texture2D<uint> ssaoMap = ResourceDescriptorHeap[g_ssaoMapIndex];
    uint screenWidth, screenHeight;
    ssaoMap.GetDimensions(screenWidth, screenHeight);
    uint ssao = ssaoMap.Sample(linearSampler, float2(input.position.x / screenWidth, input.position.y / screenHeight));
    float ao = min(aoMap.Sample(linearSampler, input.tex).r, (ssao / 255.f));
    //ao *= ao;
    float metalness = metalnessMap.Sample(linearSampler, input.tex).r;
    float roughness = roughnessMap.Sample(linearSampler, input.tex).r;

    float thickness = 0.01;
    float sharpness = 1;
    float refractiveIndex = 1;


    float3 directSSS = directionalLightSSS(g_directionalLight,
                                           N,
                                           V,
                                           thickness,
                                           sharpness,
                                           refractiveIndex) * 0.1 * albedo;

    float3 directRadiance = DirectionalLightContributionWithSSS(
        N, V, albedo, metalness, roughness, g_directionalLight,
        shadowMap, shadowMapSampler, shadowTransform, input.worldPosition, directSSS
    );

    float3 pointRadiance = float3(0, 0, 0);

    for (int i = 0; i < g_numPointLights; i++)
    {
        float3 pointSSS = pointLightSSS(g_pointLights[i],
                                  input.worldPosition,
                                  N,
                                  V,
                                  thickness,
                                  sharpness,
                                  refractiveIndex) * 0.1 * albedo;

        pointRadiance += PointLightContributionWithSSS(
            N, V, albedo, metalness, roughness, g_pointLights[i],
            pointShadowMaps, shadowMapSampler, input.worldPosition, pointSSS
        );
    }

    float3 ambient = IndirectLighting(
        environmentMap, linearSampler,
        N, V, albedo, ao, metalness, roughness
    );

    float3 outputColour = ambient
        + directRadiance
        + pointRadiance
        + emissiveRadiance;

    outputColour = ApplyFog(outputColour, input.worldPosition, cameraPosition);

    return float4(outputColour, albedoSample.a);
}