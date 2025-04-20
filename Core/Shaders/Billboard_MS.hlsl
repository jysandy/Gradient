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
    uint g_useCameraDirectionForCulling;
    float pad[2];
};

struct VertexType
{
    float4 position : SV_Position;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
    float3 worldPosition : POSITION1;
};

static const float3 bbVertices[] =
{
    // Upward face    
    float3(-0.5, 0, 0.5),   // Bottom left
    float3(0.5, 0, 0.5),    // Bottom right
    float3(0.5, 0, -0.5),   // Top right
    float3(-0.5, 0, -0.5),   // Top left
};

static const float2 bbTexCoords[] =
{
    float2(0, 1),
    float2(1, 1),
    float2(1, 0),
    float2(0, 0)
};

// Normals are (0, 1, 0) for the upward face and (0, -1, 0) 
// for the downward face

static const uint3 bbIndices[] =
{
    // Upward face
    uint3(0, 3, 1),
    uint3(1, 3, 2),
};


#define NUM_THREADS 32
#define VERTS_PER_INSTANCE 4
#define TRIS_PER_INSTANCE 2

[numthreads(NUM_THREADS, 1, 1)]
[outputtopology("triangle")]
void main( 
    in uint gtid : SV_GroupThreadID,
    in uint gid : SV_GroupID,
    out indices uint3 tris[NUM_THREADS / 2], // 2 tris per mesh * 8 meshes
    out vertices VertexType verts[NUM_THREADS])  // 4 verts per mesh * 8 meshes 
{
    const uint instancesPerGroup = NUM_THREADS / VERTS_PER_INSTANCE;
    const uint numTris = NUM_THREADS / 2;
    
    const uint trianglesPerInstance = TRIS_PER_INSTANCE;
    const uint vertsPerInstance = VERTS_PER_INSTANCE;
    
    // Set the output count -- this is not allowed to be divergent. Probably    
    uint numInstancesEmitted = min(instancesPerGroup, g_numInstances - gid * instancesPerGroup);
    SetMeshOutputCounts(vertsPerInstance * numInstancesEmitted,
        trianglesPerInstance * numInstancesEmitted);
    
    uint threadsPerInstance = NUM_THREADS / instancesPerGroup;
    
    // TODO: Rewrite using SV_GroupIndex or whatever
    uint localInstanceIndex = gtid / threadsPerInstance;
    uint instanceIndex = gid * instancesPerGroup + localInstanceIndex;
    
    if (instanceIndex < g_numInstances)
    {
        uint vertIndex = gtid % vertsPerInstance;
        
        VertexType output;
        output.worldPosition = bbVertices[vertIndex] * float3(g_cardWidth, 1, g_cardHeight); // scale the position by the card scale
        output.tex = bbTexCoords[vertIndex];
        //output.normal = vertIndex < 4 ? float3(0, 1, 0) : float3(0, -1, 0);
        
        // TODO: should this go into groupshared memory?
        InstanceData instance = Instances[instanceIndex];
        
        // Apply transforms
        float4x4 instanceTransform = QuatTo4x4(Instances[instanceIndex].RotationQuat);
        instanceTransform._41_42_43 = Instances[instanceIndex].LocalPositionWithPad.xyz;
        float4x4 worldMatrix = mul(instanceTransform, g_parentWorldMatrix);
       
        output.worldPosition = mul(float4(output.worldPosition, 1), worldMatrix).xyz;
        output.position = mul(float4(output.worldPosition, 1), g_viewProj);
        
        float3 rotatedFrontNormal = mul(float4(0, 1, 0, 0), worldMatrix).xyz;
        
        bool frontFacing = g_useCameraDirectionForCulling ? 
            dot(rotatedFrontNormal, g_cameraDirection) < 0.f :            
            dot(rotatedFrontNormal, worldMatrix._41_42_43 - g_cameraPosition) < 0.f;
        
        if (frontFacing)
        {
            // Resolve sub-UVs
            output.tex.x = lerp(instance.TexcoordUAndVRange.x,
            instance.TexcoordUAndVRange.y,
            output.tex.x);
            output.tex.y = lerp(instance.TexcoordUAndVRange.z,
            instance.TexcoordUAndVRange.w,
            output.tex.y);


            output.normal = rotatedFrontNormal;
        
        
            verts[gtid] = output;
        }
        else
        {
            // Resolve sub-UVs, flipped laterally
            output.tex.x = lerp(instance.TexcoordUAndVRange.x,
                                instance.TexcoordUAndVRange.y,
                                1 - output.tex.x);
            output.tex.y = lerp(instance.TexcoordUAndVRange.z,
                                instance.TexcoordUAndVRange.w,
                                output.tex.y);
            
            output.normal = -rotatedFrontNormal;
        
            verts[gtid] = output;
        }

        
        // Emit indices
        if (gtid < numTris)
        {
            uint indexThreadsPerInstance = numTris / instancesPerGroup;
            uint localInstanceIndex = gtid / indexThreadsPerInstance;
            uint instanceIndex = gid * instancesPerGroup + localInstanceIndex;
            
            InstanceData instance = Instances[instanceIndex];
            
            // Apply transforms
            float4x4 instanceTransform = QuatTo4x4(Instances[instanceIndex].RotationQuat);
            instanceTransform._41_42_43 = Instances[instanceIndex].LocalPositionWithPad.xyz;
            float4x4 worldMatrix = mul(instanceTransform, g_parentWorldMatrix);
       
            float3 rotatedFrontNormal = mul(float4(0, 1, 0, 0), worldMatrix).xyz;
        
            bool frontFacing = g_useCameraDirectionForCulling ?
                dot(rotatedFrontNormal, g_cameraDirection) < 0.f :
                dot(rotatedFrontNormal, worldMatrix._41_42_43 - g_cameraPosition) < 0.f;

            if (frontFacing)
            {
                tris[gtid] = (vertsPerInstance * uint3(localInstanceIndex, localInstanceIndex, localInstanceIndex))
                    + bbIndices[gtid % trianglesPerInstance];
            }
            else
            {
                tris[gtid] = (vertsPerInstance * uint3(localInstanceIndex, localInstanceIndex, localInstanceIndex))
                    + bbIndices[gtid % trianglesPerInstance].xzy;
            }
        }
    }
}