#include "Quaternion.hlsli"

cbuffer MatrixBuffer : register(b0, space0)
{
    matrix g_parentWorldMatrix;
    matrix g_viewProj;
};

struct InstanceData
{
    float4 LocalPositionWithPad;
    Quaternion RotationQuat;
    float4 TexcoordUAndVRange;
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

//float4x4 GetTransform(InstanceData instance)
//{
//    float4x4 transform = QuatTo4x4(instance.RotationQuat);
//    transform._41_42_43 = instance.LocalPositionWithPad.xyz;
    
//    return transform;
//}

OutputType main(InputType input, uint InstanceID : SV_InstanceID)
{
    OutputType output;
    
    // Instance data is fetched per-vertex here. 
    // TODO: Fetch instance data per instance instead using a mesh shader.
    InstanceData instance = Instances[InstanceID];

    // Resolve sub-UVs
    output.tex.x = lerp(instance.TexcoordUAndVRange.x,
        instance.TexcoordUAndVRange.y,
        input.tex.x);
    output.tex.y = lerp(instance.TexcoordUAndVRange.z,
        instance.TexcoordUAndVRange.w,
        input.tex.y);
    
    float4x4 instanceTransform = QuatTo4x4(instance.RotationQuat);
    instanceTransform._41_42_43 = instance.LocalPositionWithPad.xyz;
    
    float4x4 worldMatrix = mul(instanceTransform, g_parentWorldMatrix);

    float4 worldPosition = mul(float4(input.position, 1), worldMatrix);
    output.normal = mul(float4(input.normal, 0), worldMatrix);
    output.worldPosition = worldPosition.xyz;
    
    output.position = mul(worldPosition, g_viewProj);

    return output;
}