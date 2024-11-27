#ifndef __CUBE_MAP_HLSLI__
#define __CUBE_MAP_HLSLI__

float3 cubeMapSampleVector(float3 sampleVec)
{
    float3 ret = sampleVec;
    ret.z = -sampleVec.z;
    return ret;
}

#endif
