#ifndef __SHADOW_MAPPING_HLSLI__
#define __SHADOW_MAPPING_HLSLI__

#include "CubeMap.hlsli"
#include "LightStructs.hlsli"

float calculateShadowFactor(
    Texture2D shadowMap,
    SamplerComparisonState shadowMapSampler,
    float4x4 shadowTransform,
    float3 worldPosition)
{
    // TODO: Move this multiplication to the vertex shader
    float4 shadowUV = mul(float4(worldPosition, 1.f), shadowTransform);
    
    shadowUV.xyz /= shadowUV.w;
    
    if (shadowUV.x < 0
        || shadowUV.x > 1
        || shadowUV.y < 0
        || shadowUV.y > 1
        || shadowUV.z < 0 
        || shadowUV.z > 1)
    {
        return 1.f;
    }
    
    // DEBUG: Disabling PCF
    //return saturate(shadowMap.SampleCmpLevelZero(shadowMapSampler,
    //        shadowUV.xy,
    //        shadowUV.z));
    
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
    
    // TODO: Pass the shadow map resolution through
    const float dx = 1.f / 4096.f;
    
    // Use a dithered pattern to obtain a result similar to 16 
    // samples per pixel.
    // https://developer.nvidia.com/gpugems/gpugems/part-ii-lighting-and-shadows/chapter-11-shadow-map-antialiasing
    //const float2 offsets[4] =
    //{
    //    float2(-1.5, 0.5), float2(0.5, 0.5),
    //    float2(-1.5, -1.5), float2(0.5, -1.5)
    //};
    
    //float shadowFactor = 0.f;
    //[unroll]
    //for (int i = 0; i < 4; ++i)
    //{
    //    float2 finalOffsets = offsets[i] * dx;
    //    shadowFactor += saturate(shadowMap.SampleCmpLevelZero(shadowMapSampler,
    //        shadowUV.xy + finalOffsets,
    //        shadowUV.z + dot(finalOffsets, zPartials)
    //    ).r);
    //}
    
    //return shadowFactor / 4.f;
    
    const float2 offsets[25] =
    {
        float2(-2, -2), float2(-1, -2), float2(0, -2), float2(1, -2),  float2(2, -2),
        float2(-2, -1), float2(-1, -1), float2(0, -1), float2(1, -1),  float2(2, -1),
        float2(-2, 0), float2(-1, 0), float2(0, 0), float2(1, 0),     float2(2, 0),
        float2(-2, 1), float2(-1, 1), float2(0, 1), float2(1, 1),     float2(2, 1),
        float2(-2, 2), float2(-1, 2), float2(0, 2), float2(1, 2),     float2(2, 2)
        
    };
    
    float shadowFactor = 0.f;
    for (int i = 0; i < 25; ++i)
    {
        float2 finalOffsets = offsets[i] * dx;
        shadowFactor += saturate(shadowMap.SampleCmpLevelZero(shadowMapSampler,
            shadowUV.xy + finalOffsets,
            shadowUV.z + dot(finalOffsets, zPartials)
        ).r);
    }
    
    return shadowFactor / 25.f;
}

float calculateShadowFactorNoLargeKernel(
    Texture2D shadowMap,
    SamplerComparisonState shadowMapSampler,
    float4x4 shadowTransform,
    float3 worldPosition)
{
    // TODO: Move this multiplication to the vertex shader
    float4 shadowUV = mul(float4(worldPosition, 1.f), shadowTransform);
    
    shadowUV.xyz /= shadowUV.w;
    
    if (shadowUV.x < 0
        || shadowUV.x > 1
        || shadowUV.y < 0
        || shadowUV.y > 1
        || shadowUV.z < 0
        || shadowUV.z > 1)
    {
        return 1.f;
    }
    
    return saturate(shadowMap.SampleCmpLevelZero(shadowMapSampler,
            shadowUV.xy,
            shadowUV.z));
}

float cubeShadowFactor(TextureCubeArray shadowMaps,
    SamplerComparisonState shadowMapSampler,
    PointLight pointLight,
    float3 worldPosition
    )
{
    float3 uvw = worldPosition - pointLight.position;

    if (dot(uvw, uvw) > pointLight.maxRange * pointLight.maxRange)
    {
        // Early return if the position is out of range of the light.
        return 1.f;
    }
    
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
        = mul(float4(worldPosition, 1.f), pointLight.shadowTransforms[shadowTransformIndex]);
    float depth = transformed.z / transformed.w;
        
    if (depth > 1.f || depth < 0.f)
        return 1.f;
    
    return shadowMaps.SampleCmpLevelZero(shadowMapSampler,
        float4(cubeMapSampleVector(uvw),
                pointLight.shadowCubeIndex), depth);
}

#endif