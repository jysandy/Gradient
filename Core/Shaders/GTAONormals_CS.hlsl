#include "XeGTAO.h"
#include "XeGTAO.hlsli"

cbuffer GTAOConstantBuffer : register(b0)
{
    GTAOConstants g_GTAOConsts;
}

Texture2D<float> g_srcRawDepth : register(t0);
RWTexture2D<uint> g_outNormalmap : register(u0);

// g_samplerPointClamp is a sampler with D3D12_FILTER_MIN_MAG_MIP_POINT filter and D3D12_TEXTURE_ADDRESS_MODE_CLAMP addressing mode
SamplerState g_samplerPointClamp : register(s0);

[numthreads(XE_GTAO_NUMTHREADS_X, XE_GTAO_NUMTHREADS_Y, 1)]
void GTAOGenerateNormals(const uint2 pixCoord : SV_DispatchThreadID)
{
    float3 viewspaceNormal = XeGTAO_ComputeViewspaceNormal(pixCoord, g_GTAOConsts, g_srcRawDepth, g_samplerPointClamp);

    // pack from [-1, 1] to [0, 1] and then to R11G11B10_UNORM
    g_outNormalmap[pixCoord] = XeGTAO_FLOAT3_to_R11G11B10_UNORM(saturate(viewspaceNormal * 0.5 + 0.5));
}