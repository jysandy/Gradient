
Texture2D texture0 : register(t0);
SamplerState sampler0 : register(s0);

Texture2D shadowMap : register(t1);
SamplerComparisonState shadowMapSampler : register(s1);

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

/*
cbuffer LightBuffer : register(b0)
{
    PointLight pointLights[3];
    SpotLight spotLight;
    DirectionalLight directionalLight;
    uint numPointLights;
};
*/

cbuffer Constants : register(b1)
{
    float3 cameraPosition;
    float pad;
    float4x4 shadowTransform;
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

float4 calculateDirectionalLighting(DirectionalLight light, float3 worldPosition, float3 normal)
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
    
    return light.ambient + nonAmbient;
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

float4 main(InputType input) : SV_TARGET
{
    input.normal = normalize(input.normal);
    
    float4 textureColour;
    
    textureColour = texture0.Sample(sampler0, input.tex);
    
    /*
    float4 pointLightColour = { 0.f, 0.f, 0.f, 0.f };
    
    for (int i = 0; i < numPointLights; i++)
    {
        pointLightColour += calculatePointLighting(pointLights[i], input.worldPosition, input.normal);
    }
    
    float4 spotLightColour = calculateSpotLighting(spotLight, input.worldPosition, input.normal);
    */
    
    DirectionalLight directionalLight;
    directionalLight.diffuse = float4(0.8f, 0.8f, 0.6f, 1.f);
    directionalLight.ambient = float4(0.1f, 0.1f, 0.1f, 1.f);
    directionalLight.specular = float4(0.8f, 0.8f, 0.4f, 1.f);
    directionalLight.direction = float3(-0.25f, -0.3f, 1.f);
    
    float4 directionalLightColour = calculateDirectionalLighting(directionalLight, input.worldPosition, input.normal);
    
    //float4 outputColour = saturate(pointLightColour + spotLightColour + directionalLightColour) * textureColour;
    
    float4 outputColour = saturate(directionalLightColour) * textureColour;
	
    return outputColour;
}
