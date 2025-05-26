#include "XeGTAO.h"
#include "XeGTAO.hlsli"

cbuffer GTAOConstantBuffer : register(b0)
{
    GTAOConstants g_GTAOConsts;
}

Texture2D<lpfloat> g_srcWorkingDepth : register(t0); // viewspace depth with MIPs, output by XeGTAO_PrefilterDepths16x16 and consumed by XeGTAO_MainPass
Texture2D<uint> g_srcNormalmap : register(t1); // source normal map

RWTexture2D<uint> g_outWorkingAOTerm : register(u0); // output AO term (includes bent normals if enabled - packed as R11G11B10 scaled by AO)
RWTexture2D<unorm float> g_outWorkingEdges : register(u1); // output depth-based edges used by the denoiser

// g_samplerPointClamp is a sampler with D3D12_FILTER_MIN_MAG_MIP_POINT filter and D3D12_TEXTURE_ADDRESS_MODE_CLAMP addressing mode
SamplerState g_samplerPointClamp : register(s0);

// From https://github.com/GameTechDev/XeGTAO/blob/master/Source/Rendering/Shaders/vaGTAO.hlsl#L53
lpfloat3 LoadNormal(int2 pos)
{
#if 1
    // special decoding for external normals stored in 11_11_10 unorm - modify appropriately to support your own encoding 
    uint packedInput = g_srcNormalmap.Load(int3(pos, 0)).x;
    float3 unpackedOutput = XeGTAO_R11G11B10_UNORM_to_FLOAT3(packedInput);
    float3 normal = normalize(unpackedOutput * 2.0.xxx - 1.0.xxx);
#else 
    // example of a different encoding
    float3 encodedNormal = g_srcNormalmap.Load(int3(pos, 0)).xyz;
    float3 normal = normalize(encodedNormal * 2.0.xxx - 1.0.xxx);
#endif

#if 0 // compute worldspace to viewspace here if your engine stores normals in worldspace; if generating normals from depth here, they're already in viewspace
    normal = mul( (float3x3)g_globals.View, normal );
#endif

    return (lpfloat3) normal;
}

// From https://github.com/GameTechDev/XeGTAO/blob/master/Source/Rendering/Shaders/vaGTAO.hlsl#L74
lpfloat2 SpatioTemporalNoise(uint2 pixCoord, uint temporalIndex)    // without TAA, temporalIndex is always 0
{
    float2 noise;
#if 1   // Hilbert curve driving R2 (see https://www.shadertoy.com/view/3tB3z3)
#ifdef XE_GTAO_HILBERT_LUT_AVAILABLE // load from lookup texture...
        uint index = g_srcHilbertLUT.Load( uint3( pixCoord % 64, 0 ) ).x;
#else // ...or generate in-place?
    uint index = HilbertIndex(pixCoord.x, pixCoord.y);
#endif
    index += 288 * (temporalIndex % 64); // why 288? tried out a few and that's the best so far (with XE_HILBERT_LEVEL 6U) - but there's probably better :)
    // R2 sequence - see http://extremelearning.com.au/unreasonable-effectiveness-of-quasirandom-sequences/
    return lpfloat2(frac(0.5 + index * float2(0.75487766624669276005, 0.5698402909980532659114)));
#else   // Pseudo-random (fastest but looks bad - not a good choice)
    uint baseHash = Hash32( pixCoord.x + (pixCoord.y << 15) );
    baseHash = Hash32Combine( baseHash, temporalIndex );
    return lpfloat2( Hash32ToFloat( baseHash ), Hash32ToFloat( Hash32( baseHash ) ) );
#endif
}

// From https://github.com/GameTechDev/XeGTAO/blob/master/Source/Rendering/Shaders/vaGTAO.hlsl#L118
[numthreads(XE_GTAO_NUMTHREADS_X, XE_GTAO_NUMTHREADS_Y, 1)]
void GTAOMainPassHigh(const uint2 pixCoord : SV_DispatchThreadID)
{
    XeGTAO_MainPass(pixCoord, 3, 3, SpatioTemporalNoise(pixCoord, g_GTAOConsts.NoiseIndex), LoadNormal(pixCoord), g_GTAOConsts, g_srcWorkingDepth, g_samplerPointClamp, g_outWorkingAOTerm, g_outWorkingEdges);
}