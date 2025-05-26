#include "XeGTAO.h"
#include "XeGTAO.hlsli"

cbuffer GTAOConstantBuffer : register(b0)
{
    GTAOConstants g_GTAOConsts;
}

Texture2D<float> g_srcRawDepth : register(t0);

RWTexture2D<lpfloat> g_outWorkingDepthMIP0 : register(u0);
RWTexture2D<lpfloat> g_outWorkingDepthMIP1 : register(u1);
RWTexture2D<lpfloat> g_outWorkingDepthMIP2 : register(u2);
RWTexture2D<lpfloat> g_outWorkingDepthMIP3 : register(u3);
RWTexture2D<lpfloat> g_outWorkingDepthMIP4 : register(u4);

// g_samplerPointClamp is a sampler with D3D12_FILTER_MIN_MAG_MIP_POINT filter and D3D12_TEXTURE_ADDRESS_MODE_CLAMP addressing mode
SamplerState g_samplerPointClamp : register(s0);

[numthreads(8, 8, 1)] // <- hard coded to 8x8; each thread computes 2x2 blocks so processing 16x16 block: Dispatch needs to be called with (width + 16-1) / 16, (height + 16-1) / 16
void GTAOPrefilterDepths(uint2 dispatchThreadID : SV_DispatchThreadID, uint2 groupThreadID : SV_GroupThreadID)
{
    XeGTAO_PrefilterDepths16x16(dispatchThreadID, 
        groupThreadID, 
        g_GTAOConsts, 
        g_srcRawDepth, 
        g_samplerPointClamp, 
        g_outWorkingDepthMIP0, 
        g_outWorkingDepthMIP1, 
        g_outWorkingDepthMIP2, 
        g_outWorkingDepthMIP3, 
        g_outWorkingDepthMIP4);
}