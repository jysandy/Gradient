static const float PI = 3.14159265359;

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

struct DirectionalLight
{
    float4 colour;
    float3 direction;
};

cbuffer LightBuffer : register(b0)
{
    DirectionalLight directionalLight;
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

float calculateShadowFactor(float3 worldPosition)
{
    // TODO: Move this multiplication to the vertex shader
    float4 shadowUV = mul(float4(worldPosition, 1.f), shadowTransform);
    
    shadowUV.xyz /= shadowUV.w;
    
    float3 dpdx = ddx(shadowUV).xyz;
    float3 dpdy = ddy(shadowUV).xyz;
    
    float2x2 right =
    {
        dpdy.y, -dpdy.x,
        -dpdx.y, dpdx.x
    };
    
    float constant = 1.f / (dpdx.x * dpdy.y - dpdy.x * dpdx.y);
    float2 zPartials = constant * mul(float2(dpdx.z, dpdy.z), right);
    
    const float dx = 1.f / 1024.f;

    const float2 offsets[9] =
    {
        float2(-dx, -dx), float2(0.f, -dx), float2(dx, -dx),
        float2(-dx, 0.f), float2(0.f, 0.f), float2(dx, 0.f),
        float2(-dx, +dx), float2(0.f, +dx), float2(dx, +dx)
    };
    
    float shadowFactor = 0.f;
    [unroll]
    for (int i = 0; i < 9; ++i)
    {
        shadowFactor += saturate(shadowMap.SampleCmpLevelZero(shadowMapSampler,
            shadowUV.xy + offsets[i],
            shadowUV.z + dot(offsets[i], zPartials)
        ).r);
    }
    
    return shadowFactor / 9.f;
}

// Construct a cotangent frame for normal mapping as described in 
// http://www.thetenthplanet.de/archives/1180
float3x3 cotangentFrame(float3 normal, float3 worldPosition, float2 tex)
{
    float3 dp1 = ddx(worldPosition);
    float3 dp2 = ddy(worldPosition);
    float2 duv1 = ddx(tex);
    float2 duv2 = ddy(tex);
    
    float3 dp2perp = cross(dp2, normal);
    float3 dp1perp = cross(normal, dp1);
    float3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    float3 B = dp2perp * duv1.y + dp1perp * duv2.y;
    
    float invmax = rsqrt(max(dot(T, T), dot(B, B)));
    return float3x3(T * invmax, B * invmax, normal);
}

float3 perturbNormal(float3 N, float3 worldPosition, float2 tex)
{
    float3 map = normalMap.SampleLevel(linearSampler, tex, 0).xyz;
    map.xy = map.xy * 2.f - 1.f;
    // Using right-handed coordinates, and assuming green up
    map.x = -map.x;
    map.y = -map.y;
    
    map.z = sqrt(1.f - dot(map.xy, map.xy));
    
    float3x3 TBN = cotangentFrame(N, worldPosition, tex);
    return normalize(mul(map, TBN));
}

// PBR lighting -----------------------

float3 directionalLightRadiance(DirectionalLight dlight)
{
    return 10 * dlight.colour.rgb;
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
    
    return saturate(F0 + (1.f - F0) * pow(saturate(1.f - cosTheta), 5.f));
}

float distributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.f);
    float NdotH2 = NdotH * NdotH;
	
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

float geometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}

float geometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.f);
    float NdotL = max(dot(N, L), 0.f);
    float ggx2 = geometrySchlickGGX(NdotV, roughness);
    float ggx1 = geometrySchlickGGX(NdotL, roughness);
	
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
    float3 radiance
)
{
    float3 F = fresnelSchlick(H, V, albedo, metalness);
    float D = distributionGGX(N, H, roughness);
    float G = geometrySmith(N, V, L, roughness);
    
    float NdotL = max(dot(N, L), 0.f);
    
    float3 numerator = D * G * F;
    float denominator = 4.f * max(dot(N, V), 0.f) * NdotL + 0.0001;
    
    float3 specular = numerator / denominator;
    
    float3 kS = F;
    float3 kD = float3(1.f, 1.f, 1.f) - kS;
    kD *= 1.f - metalness;
    
    float3 outgoingRadiance = (kD * (albedo / PI) + specular)
        * radiance
        * NdotL;
    
    return clamp(outgoingRadiance,
                 float3(0, 0, 0),
                 float3(50, 50, 50));
}

float3 cookTorranceDirectionalLight(float3 N,
    float3 V,
    float3 albedo,
    float metalness,
    float roughness,
    DirectionalLight light)
{
    float3 L = normalize(-directionalLight.direction);
    float3 H = normalize(V + L);
    
    float3 radiance = directionalLightRadiance(light);
    
    return cookTorranceRadiance(
        N, V, L, H, albedo, metalness, roughness, radiance
    );
}

float3 sampleEnvironmentMap(float3 sampleVec)
{
    sampleVec.z = -sampleVec.z;
    return environmentMap.Sample(linearSampler, sampleVec).rgb;
}

float3 cookTorranceEnvironmentMap(float3 N,
    float3 V, 
    float3 albedo,
    float ao, 
    float metalness,
    float roughness)
{
    float3 L = normalize(reflect(-V, N));
    float3 H = normalize(V + L);
    
    float3 radiance = 0.1 * sampleEnvironmentMap(L);
    
    return ao * cookTorranceRadiance(
        N, V, L, H, albedo, metalness, roughness, radiance
    );
}

// ------------------------------------

float4 main(InputType input) : SV_TARGET
{
    input.normal = normalize(input.normal);
    
    float3 N = perturbNormal(input.normal, input.worldPosition, input.tex);
    float3 V = normalize(cameraPosition - input.worldPosition);
    
    
    float4 albedoSample = albedoMap.Sample(anisotropicSampler, input.tex);
    float3 albedo = albedoSample.rgb;
    float ao = aoMap.Sample(linearSampler, input.tex).r;
    float metalness = metalnessMap.Sample(linearSampler, input.tex).r;
    float roughness = roughnessMap.Sample(linearSampler, input.tex).r;
    
    float3 directRadiance = cookTorranceDirectionalLight(
        N, V, albedo, metalness, roughness, directionalLight
    );
    
    float3 ambient = cookTorranceEnvironmentMap(
        N, V, albedo, ao, metalness, roughness
    );
    
    float shadowFactor = calculateShadowFactor(input.worldPosition);
    
    float3 outputColour = ambient + shadowFactor * directRadiance;
    
    return float4(outputColour, albedoSample.a);
}