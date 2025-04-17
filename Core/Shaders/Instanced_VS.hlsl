#include "Quaternion.hlsli"

cbuffer MatrixBuffer : register(b0, space0)
{
    matrix g_parentWorldMatrix;
    matrix g_viewMatrix;
    matrix g_projectionMatrix;
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

float4x4 GetTransform(InstanceData instance)
{
    float4x4 transform = QuatTo4x4(instance.RotationQuat);
    transform._41_42_43 = instance.LocalPositionWithPad.xyz;
    
    return transform;
}

OutputType main(InputType input, uint InstanceID : SV_InstanceID)
{
    OutputType output;
    
    // Instance data is fetched per-vertex here. 
    // TODO: Fetch instance data per instance instead using a mesh shader.
    InstanceData instance = Instances[InstanceID];
    
    float4x4 instanceTransform = GetTransform(instance);
    float4x4 viewProjection = mul(g_viewMatrix, g_projectionMatrix);
    float4x4 worldMatrix = mul(instanceTransform, g_parentWorldMatrix);

    float4 worldPosHomo = mul(float4(input.position, 1), worldMatrix);
    output.position = mul(worldPosHomo, viewProjection);

    // Resolve sub-UVs
    output.tex.x = lerp(instance.TexcoordUAndVRange.x,
        instance.TexcoordUAndVRange.y,
        input.tex.x);
    output.tex.y = lerp(instance.TexcoordUAndVRange.z,
        instance.TexcoordUAndVRange.w,
        input.tex.y);
    
    output.normal = mul(input.normal, (float3x3) worldMatrix);
    output.normal = normalize(output.normal);
	
    output.worldPosition = worldPosHomo.xyz / worldPosHomo.w;

    return output;
}