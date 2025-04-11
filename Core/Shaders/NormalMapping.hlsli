#ifndef __NORMAL_MAPPING_HLSLI__
#define __NORMAL_MAPPING_HLSLI__

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

float3 perturbNormal(
    Texture2D<float4> normalMap,
    SamplerState inSampler,
    float3 N, 
    float3 worldPosition, 
    float2 tex)
{
    float3 map = normalMap.Sample(inSampler, tex).xyz;
    map.xy = map.xy * 2.f - 1.f;
    // Using right-handed coordinates, and assuming green up
    map.x = -map.x;
    map.y = -map.y;
    
    map.z = sqrt(1.f - dot(map.xy, map.xy));
    
    float3x3 TBN = cotangentFrame(N, worldPosition, tex);
    return normalize(mul(map, TBN));
}

#endif