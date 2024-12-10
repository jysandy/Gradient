#include "CubeMap.hlsli"

float calculateShadowFactor(
    Texture2D shadowMap,
    SamplerComparisonState shadowMapSampler,
    float4x4 shadowTransform,
    float3 worldPosition)
{
    // TODO: Move this multiplication to the vertex shader
    float4 shadowUV = mul(float4(worldPosition, 1.f), shadowTransform);
    
    shadowUV.xyz /= shadowUV.w;
    
    // Large kernel PCF. Partial derivatives are needed to 
    // estimate the depths of the adjacent samples.
    
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

float cubeShadowFactor(TextureCubeArray shadowMaps,
    SamplerComparisonState shadowMapSampler,
    float3 lightPosition,
    float3 worldPosition,
    float4x4 shadowTransforms[6],
    uint cubeIndex
    )
{
    float3 uvw = worldPosition - lightPosition;
    
    float3 unitVectors[6] =
    {
        {1, 0, 0 },
        {-1, 0, 0 },
        {0, 1, 0 },
        {0, -1, 0 },
        {0, 0, 1 },
        {0, 0, -1 }
    };
    
    float maxCosTheta = 0.f;
    uint shadowTransformIndex = 0;
    float3 uvwUnit = normalize(uvw);
    
    // Figure out which face of the cubemap needs to be used.
    // Take the dot product with each cardinal direction, 
    // and choose the direction with the highest dot product.
    for (int i = 0; i < 6; i++)
    {
        float cosTheta = dot(uvwUnit, unitVectors[i]);
        if (cosTheta > maxCosTheta)
        {
            maxCosTheta = cosTheta;
            shadowTransformIndex = i;
        }
    }

    float4 transformed 
        = mul(float4(worldPosition, 1.f), shadowTransforms[shadowTransformIndex]);
    float depth = transformed.z / transformed.w;
        
    if (depth > 1.f || depth < 0.f)
        return 1.f;
    
    return shadowMaps.SampleCmpLevelZero(shadowMapSampler,
        float4(cubeMapSampleVector(uvw),
                cubeIndex), depth);
}