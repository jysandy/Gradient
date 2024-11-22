#ifndef __PBR_LIGHTING_HLSLI__
#define __PBR_LIGHTING_HLSLI__

#include "LightStructs.hlsli"

static const float PI = 3.14159265359;

float3 directionalLightIrradiance(DirectionalLight dlight)
{
    return dlight.irradiance * dlight.colour.rgb;
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
    
    return F0 + (1.f - F0) * pow(1.f - cosTheta, 5.f);
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

float3 sampleEnvironmentMap(TextureCube environmentMap,
    SamplerState linearSampler,
    float3 sampleVec)
{
    sampleVec.z = -sampleVec.z;
    return environmentMap.Sample(linearSampler, sampleVec).rgb;
}

float3 cookTorranceEnvironmentMap(
    TextureCube environmentMap,
    SamplerState linearSampler,
    float3 N,
    float3 V,
    float3 albedo,
    float ao,
    float metalness,
    float roughness)
{
    float3 L = normalize(reflect(-V, N));
    float3 H = normalize(V + L);
    
    float3 radiance = 1 * sampleEnvironmentMap(
        environmentMap,
        linearSampler,
        L);
    
    float3 ct = cookTorranceRadiance(
        N, V, L, H, albedo, metalness, roughness, radiance, false
    );
    
    return ao * ct;
}

#endif