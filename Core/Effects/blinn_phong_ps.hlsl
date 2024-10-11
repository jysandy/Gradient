// Light pixel shader
// Calculate diffuse lighting for a single directional light (also texturing)

Texture2D texture0 : register(t0);
SamplerState sampler0 : register(s0);

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

cbuffer CameraBuffer : register(b1)
{
    float3 cameraPosition;
    float pad;
}
*/

float3 cameraPosition = { 0.f, 10.f, 25.f };


struct InputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
    float3 worldPosition : POSITION1;
};

float4 calculateDirectionalLighting(DirectionalLight light, float3 worldPosition, float3 normal)
{
    float3 toLight = -normalize(light.direction);
    float intensity = saturate(dot(normal, toLight));
    float4 colour = light.diffuse * intensity;
    
    float3 viewVector = normalize(cameraPosition - worldPosition);
    float3 halfVector = normalize((viewVector + toLight) / 2.f);
    float4 specularColour = pow(max(dot(halfVector, normal), 0), 128)
        * light.specular;
    
    return specularColour + light.ambient + colour;
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
    directionalLight.specular = float4(1.f, 1.f, 1.f, 1.f);
    directionalLight.direction = float3(0.5f, -0.5f, -1.f);
    
    float4 directionalLightColour = calculateDirectionalLighting(directionalLight, input.worldPosition, input.normal);
    
    //float4 outputColour = saturate(pointLightColour + spotLightColour + directionalLightColour) * textureColour;
    
    float4 outputColour = saturate(directionalLightColour) * textureColour;
    
	// Compensate for gamma correction. Do we need this?
    //outputColour = pow(outputColour, 1.0f / 2.2f);
	
    return outputColour;
}
