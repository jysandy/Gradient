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

cbuffer DrawParams : register(b1, space0)
{
    float3 g_cameraPosition;
    float g_cardWidth;
    float3 g_cameraDirection;
    float g_cardHeight;
    uint g_numInstances;
    float pad[3];
};

struct VertexType
{
    float4 position : SV_Position;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
    float3 worldPosition : POSITION1;
};

static const float3 bbVertices[8] =
{
    // Upward face    
    float3(-0.5, 0, 0.5),   // Bottom left
    float3(0.5, 0, 0.5),    // Bottom right
    float3(0.5, 0, -0.5),   // Top right
    float3(-0.5, 0, -0.5),   // Top left

    // Downward face
    float3(0.5, 0, 0.5),    // Bottom left (from below)
    float3(-0.5, 0, 0.5),   // Bottom right
    float3(-0.5, 0, -0.5),  // Top right
    float3(0.5, 0, -0.5)    // Top left
};

static const float2 bbTexCoords[8] =
{
    float2(0, 1),
    float2(1, 1),
    float2(1, 0),
    float2(0, 0),
    
    float2(1, 1),
    float2(0, 1),
    float2(0, 0),
    float2(1, 0)
};

// Normals are (0, 1, 0) for the upward face and (0, -1, 0) 
// for the downward face

static const uint3 bbIndices[4] =
{
    // Upward face
    uint3(0, 3, 1),
    uint3(1, 3, 2),
    
    // Downward face
    uint3(4, 6, 5),
    uint3(4, 7, 6)
};

[numthreads(32, 1, 1)]
[outputtopology("triangle")]
void main( 
    in uint gtid : SV_GroupThreadID,
    in uint gid : SV_GroupID,
    out indices uint3 tris[16],         // 4 tris per mesh * 4 meshes
    out vertices VertexType verts[32])  // 8 verts per mesh * 4 meshes 
{
    const uint instancesPerGroup = 4;
    const uint numTris = 16;
    
    const uint trianglesPerInstance = 4;
    const uint vertsPerInstance = 8;
    
    // Set the output count -- this is not allowed to be divergent. Probably    
    uint numInstancesEmitted = min(4, g_numInstances - gid * instancesPerGroup);
    SetMeshOutputCounts(vertsPerInstance * numInstancesEmitted,
        trianglesPerInstance * numInstancesEmitted);
    
    uint threadsPerInstance = 32 / instancesPerGroup;
    
    // TODO: Rewrite using SV_GroupIndex or whatever
    uint localInstanceIndex = gtid / threadsPerInstance;
    uint instanceIndex = gid * instancesPerGroup + localInstanceIndex;
    
    if (instanceIndex < g_numInstances)
    {
        uint vertIndex = gtid % vertsPerInstance;
        
        VertexType output;
        output.worldPosition = bbVertices[vertIndex] * float3(g_cardWidth, 1, g_cardHeight); // scale the position by the card scale
        output.tex = bbTexCoords[vertIndex];
        output.normal = vertIndex < 4 ? float3(0, 1, 0) : float3(0, -1, 0);
        
        // TODO: should this go into groupshared memory?
        InstanceData instance = Instances[instanceIndex];
        
        // Resolve sub-UVs
        output.tex.x = lerp(instance.TexcoordUAndVRange.x,
            instance.TexcoordUAndVRange.y,
            output.tex.x);
        output.tex.y = lerp(instance.TexcoordUAndVRange.z,
            instance.TexcoordUAndVRange.w,
            output.tex.y);
        
        // Apply transforms
        float4x4 instanceTransform = QuatTo4x4(Instances[instanceIndex].RotationQuat);
        instanceTransform._41_42_43 = Instances[instanceIndex].LocalPositionWithPad.xyz;
    
        float4x4 worldMatrix = mul(instanceTransform, g_parentWorldMatrix);
        output.worldPosition = mul(float4(output.worldPosition, 1), worldMatrix).xyz;
        output.position = mul(float4(output.worldPosition, 1), g_viewProj);
        output.normal = mul(float4(output.normal, 0), worldMatrix).xyz;
        
        verts[gtid] = output;
        
        // Emit indices
        if (gtid < numTris)
        {
            uint indexThreadsPerInstance = 16 / instancesPerGroup;
            uint localInstanceIndex = gtid / indexThreadsPerInstance;
            tris[gtid] = (8 * uint3(localInstanceIndex, localInstanceIndex, localInstanceIndex)) 
                + bbIndices[gtid % trianglesPerInstance];
        }
    }
}