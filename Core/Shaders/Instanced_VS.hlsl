#include "Quaternion.hlsli"

cbuffer MatrixBuffer : register(b0, space0)
{
    matrix g_parentWorldMatrix;
    matrix g_viewMatrix;
    matrix g_projectionMatrix;
};

struct InstanceData
{
    float3 LocalPosition;
    float pad;
    Quaternion RotationQuat;
    float2 TexcoordURange;
    float2 TexcoordVRange;
};

StructuredBuffer<InstanceData> Instances : register(t0, space0);

struct InputType
{
    float3 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 tex : TEXCOORD;
};

struct OutputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
    float3 worldPosition : POSITION1;
};

float4x4 GetTransform(InstanceData instance)
{
    float4x4 transform = QuatTo4x4(instance.RotationQuat);
    transform._41_42_43 = instance.LocalPosition;
    
    return transform;
}

OutputType main(InputType input, uint InstanceID : SV_InstanceID)
{
    OutputType output;
    
    float4x4 instanceTransform = GetTransform(Instances[InstanceID]);
    float4x4 viewProjection = mul(g_viewMatrix, g_projectionMatrix);
    float4x4 worldMatrix = mul(instanceTransform, g_parentWorldMatrix);

    float4 worldPosHomo = mul(float4(input.position, 1), worldMatrix);
    output.position = mul(worldPosHomo, viewProjection);

    // Resolve sub-UVs
    output.tex.x = lerp(Instances[InstanceID].TexcoordURange.x,
        Instances[InstanceID].TexcoordURange.y,
        input.tex.x);
    output.tex.y = lerp(Instances[InstanceID].TexcoordVRange.x,
        Instances[InstanceID].TexcoordVRange.y,
        input.tex.y);
    
    output.normal = mul(input.normal, (float3x3) worldMatrix);
    output.normal = normalize(output.normal);
	
    output.worldPosition = worldPosHomo.xyz / worldPosHomo.w;

    return output;
}