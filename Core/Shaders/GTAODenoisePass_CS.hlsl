#include "XeGTAO.h"
#include "XeGTAO.hlsli"

cbuffer GTAOConstantBuffer : register(b0)
{
    GTAOConstants g_GTAOConsts;
    bool g_isFinalPass;
}

Texture2D<uint> g_srcWorkingAOTerm : register(t0); // coming from previous pass
Texture2D<lpfloat> g_srcWorkingEdges : register(t1); // coming from previous pass
RWTexture2D<uint> g_outFinalAOTerm : register(u0); // final AO term - just 'visibility' or 'visibility + bent normals'

// g_samplerPointClamp is a sampler with D3D12_FILTER_MIN_MAG_MIP_POINT filter and D3D12_TEXTURE_ADDRESS_MODE_CLAMP addressing mode
SamplerState g_samplerPointClamp : register(s0);

[numthreads(XE_GTAO_NUMTHREADS_X, XE_GTAO_NUMTHREADS_Y, 1)]
void GTAODenoisePass(const uint2 dispatchThreadID : SV_DispatchThreadID)
{
    const uint2 pixCoordBase = dispatchThreadID * uint2(2, 1); // we're computing 2 horizontal pixels at a time (performance optimization)
    XeGTAO_Denoise(pixCoordBase, g_GTAOConsts, g_srcWorkingAOTerm, g_srcWorkingEdges, g_samplerPointClamp, g_outFinalAOTerm, g_isFinalPass);
}
