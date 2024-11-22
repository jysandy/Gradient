float calculateShadowFactor(
    Texture2D shadowMap,
    SamplerComparisonState shadowMapSampler,
    float4x4 shadowTransform,
    float3 worldPosition)
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