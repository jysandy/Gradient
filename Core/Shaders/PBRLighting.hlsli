#ifndef __PBR_LIGHTING_HLSLI__
#define __PBR_LIGHTING_HLSLI__

// Roughness-metallic PBR direct lighting, 
// as described in https://learnopengl.com/PBR/Theory
// Adapted from https://learnopengl.com/PBR/Lighting

#include "LightStructs.hlsli"
#include "ShadowMapping.hlsli"

static const float PI = 3.14159265359;

float3 directionalLightIrradiance(DirectionalLight dlight)
{
    return dlight.irradiance * dlight.colour.rgb;
}

float3 pointLightIrradiance(PointLight plight, float3 worldPosition)
{
    float d = distance(plight.position, worldPosition);
    float attenuation = max(0, plight.maxRange - d) / plight.maxRange;
    attenuation *= attenuation;
    
    return plight.irradiance * attenuation * plight.colour.rgb;
}

float3 fresnelSchlick(
    float3 H,
    float3 V,
    float3 albedo,
    float metallic)
{
    float3 F0 = float3(0.04, 0.04, 0.04);
    F0 = lerp(F0, albedo, metallic);
    float cosTheta = max(dot(H, V), 0);
    
    return F0 + (1.f - F0) * pow(saturate(1.f - cosTheta), 5.f);
}

float distributionGGX(float3 N, float3 H, float roughness)
{
    float a = max(roughness * roughness, 0.001);
    float a2 = a * a;
    
    float NdotH = max(dot(N, H), 0.001);
    
    float NdotH2 = NdotH * NdotH;
	
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

float geometrySchlickGGX(float3 N, float3 VorL, float k)
{
    // Try to avoid division by zeroes.
    // As k approaches 0, the quotient approaches 1
    if (k == 0)
        return 1;
    
    float NdotVorL = dot(N, VorL);
    
    float num = max(NdotVorL, 0.0001);
    float denom = num * (1.0 - k) + k;
	
    return num / denom;
}

float geometrySmith(float3 N, float3 V, float3 L, float roughness, bool directLighting)
{
    float k;
    if (directLighting)
    {
        float r = roughness + 1;
        k = (r * r) / 8.f;
    }
    else
    {
        k = roughness * roughness / 2;
    }
    
    float ggx2 = geometrySchlickGGX(N, V, roughness);
    float ggx1 = geometrySchlickGGX(N, L, roughness);
	
    return ggx1 * ggx2;
}

float3 cookTorranceRadiance(
    float3 N,
    float3 V,
    float3 L,
    float3 H,
    float3 albedo,
    float metalness,
    float roughness,
    float3 radiance,
    bool directLighting
)
{
    float3 F = fresnelSchlick(H, V, albedo, metalness);
    float D = distributionGGX(N, H, roughness);
    
    // When NdotH is 1 (as in the case of a cubemapped reflection),
    // this function goes to infinity at low roughness values. 
    // However, the NDF doesn't exceed 3.5 for most values of NdotH 
    // and r, so clamping it between 0 and 3.5 seems like a reasonable 
    // approximation. See: https://www.desmos.com/calculator/ppdhhd569k
    // From testing, this works well for direct lighting.
    // For indirect lighting from cubemapped reflections, we clamp the 
    // entire specular factor without F instead (see below)
    if (directLighting)
        D = clamp(D, 0, 3.5);
    
    float G = geometrySmith(N, V, L, roughness, directLighting);
    
    float NdotL = max(dot(N, L), 0.f);
    
    float3 kS = F;
    float3 kD = float3(1.f, 1.f, 1.f) - kS;
    kD *= 1.f - metalness;
    
    float3 fd = kD * (albedo / PI);
    
    float denominator = 4.f * max(dot(N, V), 0.0001f) * max(dot(N, L), 0.0001f);
    float numWithoutF = D * G;
    
    float3 specular = float3(0.f, 0.f, 0.f);
    
    // Clamp the specular factor without F to 1, 
    // for indirect lighting
    if (!directLighting)
        specular = F * saturate(numWithoutF / denominator);
    else
        specular = F * numWithoutF / denominator;
    
    float3 outgoingRadiance = (fd + specular)
        * radiance
        * NdotL;
    
    return outgoingRadiance;
}

float3 cookTorranceDirectionalLight(float3 N,
    float3 V,
    float3 albedo,
    float metalness,
    float roughness,
    DirectionalLight light)
{
    float3 L = normalize(-light.direction);
    float3 H = normalize(V + L);
    
    float3 irradiance = directionalLightIrradiance(light);
    
    return cookTorranceRadiance(
        N, V, L, H, albedo, metalness, roughness, irradiance, true
    );
}

float3 cookTorrancePointLight(float3 N,
    float3 V,
    float3 albedo,
    float metalness,
    float roughness,
    PointLight light,
    float3 worldPosition)
{
    float3 L = normalize(light.position - worldPosition);
    float3 H = normalize(V + L);
    
    float3 irradiance = pointLightIrradiance(light, worldPosition);
    
    return cookTorranceRadiance(
        N, V, L, H, albedo, metalness, roughness, irradiance, true
    );
}

float3 DirectionalLightContribution(float3 N,
    float3 V,
    float3 albedo,
    float metallic,
    float roughness,
    DirectionalLight light,
    Texture2D shadowMap,
    SamplerComparisonState shadowMapSampler,
    float4x4 shadowTransform,
    float3 worldPosition)
{
    float shadowFactor = calculateShadowFactor(shadowMap,
        shadowMapSampler,
        shadowTransform,
        worldPosition);
    
    if (shadowFactor < 0.001f)
    {
        // Don't compute the BRDF if the pixel is in shadow.
        return float3(0, 0, 0);
    }
    
    return shadowFactor * cookTorranceDirectionalLight(N,
        V,
        albedo,
        metallic,
        roughness,
        light);
}

float3 PointLightContribution(float3 N,
    float3 V,
    float3 albedo,
    float metalness,
    float roughness,
    PointLight light,
    TextureCubeArray shadowMaps,
    SamplerComparisonState shadowMapSampler,
    float3 worldPosition)
{
    float shadowFactor = cubeShadowFactor(shadowMaps,
        shadowMapSampler,
        light,
        worldPosition);
    
    if (shadowFactor < 0.001f)
    {
        return float3(0, 0, 0);
    }
    
    return shadowFactor * cookTorrancePointLight(N,
        V,
        albedo,
        metalness,
        roughness,
        light,
        worldPosition);
}

float3 sampleEnvironmentMap(TextureCube environmentMap,
    SamplerState linearSampler,
    float3 sampleVec)
{
    sampleVec.z = -sampleVec.z;
    return environmentMap.Sample(linearSampler, sampleVec).rgb;
}

float3 CookTorranceEnvironmentMap(
    TextureCube environmentMap,
    SamplerState linearSampler,
    float3 L,
    float3 N,
    float3 V,
    float3 albedo,
    float ao,
    float metalness,
    float roughness,
    float3 luminanceFactor = 4.f * float3(0.6, 1, 0.8))
{
    float3 H = normalize(V + L);
    
    float3 radiance = 1 * sampleEnvironmentMap(
        environmentMap,
        linearSampler,
        L);
    
    float luminance = dot(radiance, float3(0.2126, 0.7152, 0.0722));
    radiance += luminanceFactor * luminance 
    //* float3(0.05, 0.15, 0.1) / 0.15
    ;
    
    float3 ct = cookTorranceRadiance(
        N, V, L, H, albedo, metalness, roughness, radiance, false
    );
    
    return ao * ct;
}

float3 IndirectLighting(
    TextureCube environmentMap,
    SamplerState linearSampler,
    float3 N,
    float3 V,
    float3 albedo,
    float ao,
    float metalness,
    float roughness,
    float3 luminanceFactor = 4.f * float3(0.6, 1, 0.8))
{
    float3 L = normalize(reflect(-V, N));
                           
    float reflectionContribution = lerp(0.5, 0.95, metalness);
    
    return reflectionContribution * CookTorranceEnvironmentMap(
        environmentMap,
        linearSampler,
        L,
        N,
        V,
        albedo,
        ao,
        metalness,
        roughness,
        luminanceFactor)
        + (1.f - reflectionContribution) * CookTorranceEnvironmentMap(
        environmentMap,
        linearSampler,
        N,
        N,
        V,
        albedo,
        ao,
        metalness,
        roughness,
        luminanceFactor);
}

// Simple thickness-based subsurface scattering,
// inspired by https://www.alanzucconi.com/2017/08/30/fast-subsurface-scattering-1/
float3 subsurfaceScattering(float3 irradiance, 
                            float3 N,
                            float3 V,
                            float3 L,
                            float thickness,
                            float sharpness,
                            float refractiveIndex)
{
    float I = (1 - thickness) *
        pow(saturate(dot(V, normalize(-L + N * (refractiveIndex - 1)))), sharpness) * 0.05;
    
    return I * irradiance;
}

float3 directionalLightSSS(DirectionalLight light,
                           float3 N,
                           float3 V,
                           float thickness,
                           float sharpness,
                           float refractiveIndex)
{
    float3 irradiance = directionalLightIrradiance(light);
    float3 L = -normalize(light.direction);
    
    return subsurfaceScattering(irradiance,
                                N,
                                V,
                                L,
                                thickness,
                                sharpness,
                                refractiveIndex);
}

float3 pointLightSSS(PointLight light,
                     float3 worldPosition,
                     float3 N,
                     float3 V,
                     float thickness,
                     float sharpness,
                     float refractiveIndex)
{
    float3 irradiance = pointLightIrradiance(light, worldPosition);
    float3 L = normalize(light.position - worldPosition);
    
    return subsurfaceScattering(irradiance,
                                N,
                                V,
                                L,
                                thickness,
                                sharpness,
                                refractiveIndex);
}

#endif