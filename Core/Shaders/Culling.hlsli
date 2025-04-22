#ifndef __CULLING_HLSLI__
#define __CULLING_HLSLI__

typedef float4 BoundingSphere;

// The bounding sphere is expected to be the correct size and in world space
bool IsVisible(BoundingSphere bs, float4 Planes[6])
{
    float3 center = bs.xyz;
    float radius = bs.w;

    for (int i = 0; i < 6; ++i)
    {
        if (dot(float4(center, 1), Planes[i]) < -radius)
        {
            return false;
        }
    }
    
    return true;
}

#endif