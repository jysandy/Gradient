
Texture2D texture0 : register(t0);
SamplerState sampler0 : register(s0);

Texture2D shadowMap : register(t1);
SamplerComparisonState shadowMapSampler : register(s1);

Texture2D normalMap : register(t2);
SamplerState pointSampler : register(s2);

Texture2D aoMap : register(t3);
SamplerState linearSampler : register(s3);

struct PointLight
{
    float4 diffuse;
    float4 ambient;
    float4 specular;
    float3 position;
    float pad;
};

struct SpotLight
{
    float4 diffuse;
    float4 specular;
    float3 position;
    float power;
    float3 direction;
    float pad;
};

struct DirectionalLight
{
    float4 diffuse;
    float4 ambient;
    float4 specular;
    float3 direction;
    float pad;
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

float4 calculateDirectionalLighting(DirectionalLight light, 
    float3 worldPosition, 
    float3 normal,
    float aoFactor)
{
    float shadowFactor = calculateShadowFactor(worldPosition);
    
    float3 toLight = -normalize(light.direction);
    float intensity = saturate(dot(normal, toLight));
    float4 colour = light.diffuse * intensity;
    
    float3 viewVector = normalize(cameraPosition - worldPosition);
    float3 halfVector = normalize((viewVector + toLight) / 2.f);
    float4 specularColour = pow(max(dot(halfVector, normal), 0), 256)
        * light.specular;
  
    float4 nonAmbient = specularColour + colour;
    nonAmbient.rgb *= shadowFactor;
    
    return float4(light.ambient.rgb * aoFactor, 1.f) + nonAmbient;
}

float4 calculatePointLighting(PointLight light, float3 worldPosition, float3 normal)
{
    float3 toLight = normalize(light.position - worldPosition);
    float d = distance(worldPosition, light.position);
    float intensity = saturate(dot(normal, toLight));
    float4 colour = light.diffuse * intensity;
    
    float3 viewVector = normalize(cameraPosition - worldPosition);
    float3 halfVector = normalize((viewVector + toLight) / 2.f);
    float4 specularColour = pow(max(dot(halfVector, normal), 0), 128)
        * light.specular;
    
    return specularColour / (1 + 0.5f * d) + // weaker attenuation for the specular component
    (light.ambient + colour) / (1 + 0.2f * d + 0.1 * d * d); // some quadratic attenuation for diffuse and ambient
}

float4 calculateSpotLighting(SpotLight light, float3 worldPosition, float3 normal)
{
    float3 toLight = normalize(light.position - worldPosition);
    float intensity = saturate(dot(normal, toLight));
    float4 diffuseColour = light.diffuse * intensity;
    float spotCoefficient = max(dot(-toLight, normalize(light.direction)), 0);
    spotCoefficient = pow(spotCoefficient, light.power);
    
    float3 viewVector = normalize(cameraPosition - worldPosition);
    float3 halfVector = normalize((viewVector + toLight) / 2.f);
    float4 specularColour = pow(max(dot(halfVector, normal), 0), 128)
        * light.specular;
    
    return (diffuseColour + specularColour) * spotCoefficient;
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
    float3 map = normalMap.SampleLevel(pointSampler, tex, 0).xyz;
    map.xy = map.xy * 2.f - 1.f;
    // Using right-handed coordinates, and assuming green up
    map.x = -map.x;
    map.y = -map.y;
    
    map.z = sqrt(1.f - dot(map.xy, map.xy));
    
    float3x3 TBN = cotangentFrame(N, worldPosition, tex);
    return normalize(mul(map, TBN));
}

float4 main(InputType input) : SV_TARGET
{
    input.normal = normalize(input.normal);
    
    float3 normal = perturbNormal(input.normal, input.worldPosition, input.tex);
    float4 textureColour = texture0.Sample(sampler0, input.tex);
    float aoFactor = aoMap.Sample(linearSampler, input.tex).r;
    
    float4 directionalLightColour = calculateDirectionalLighting(directionalLight, 
        input.worldPosition, 
        normal,
        aoFactor);
    
    float4 outputColour = directionalLightColour * textureColour;	
    return outputColour;
    
    // For normal debugging
    //return float4(pow(normal * 0.5 + 0.5, 2.2), 1.f);
}
