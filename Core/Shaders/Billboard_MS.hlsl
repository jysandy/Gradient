#include "Quaternion.hlsli"
#include "Culling.hlsli"

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
    float2 pad;
    float4 g_cullingFrustumPlanes[6];
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
    float3(-0.5, 0, 0.5), // Bottom left
    float3(0.5, 0, 0.5), // Bottom right
    float3(0.5, 0, -0.5), // Top right
    float3(-0.5, 0, -0.5), // Top left
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


// 1 instance per thread!
[numthreads(NUM_THREADS, 1, 1)]
[outputtopology("triangle")]
void main(
    in uint gtid : SV_GroupIndex,
    in uint3 gid : SV_GroupID,
    out indices uint3 tris[NUM_THREADS * TRIS_PER_INSTANCE],
    out vertices VertexType verts[NUM_THREADS * VERTS_PER_INSTANCE])
{
    const uint instancesPerGroup = NUM_THREADS;
    const uint numTris = NUM_THREADS * TRIS_PER_INSTANCE;
    
    const uint trianglesPerInstance = TRIS_PER_INSTANCE;
    const uint vertsPerInstance = VERTS_PER_INSTANCE;
    
    uint localInstanceIndex = gtid;
    uint instanceIndex = gid.x * instancesPerGroup + localInstanceIndex;
    
    VertexType outputVerts[vertsPerInstance];
    uint3 outputTris[trianglesPerInstance];
    
    // Default to true to avoid flickering
    bool visible = true;
    
    if (instanceIndex < g_numInstances)
    {
        InstanceData instance = Instances[instanceIndex];
        
        // Get transform and determine if we're front-facing
        float4x4 instanceTransform = QuatTo4x4(Instances[instanceIndex].RotationQuat);
        instanceTransform._41_42_43 = Instances[instanceIndex].LocalPositionWithPad.xyz;
        float4x4 worldMatrix = mul(instanceTransform, g_parentWorldMatrix);
        
        BoundingSphere bs;
        bs.xyz = mul(float4(0, 0, 0, 1), worldMatrix).xyz;
        // Radius is the length of the diagonal
        bs.w = length(float3(g_cardWidth, 0, g_cardHeight));
        
        visible = IsVisible(bs, g_cullingFrustumPlanes);
        
        if (visible)
        {
            float3 rotatedFrontNormal = mul(float4(0, 1, 0, 0), worldMatrix).xyz;
        
            bool frontFacing = g_useCameraDirectionForCulling ?
                dot(rotatedFrontNormal, g_cameraDirection) < 0.f :
                dot(rotatedFrontNormal, worldMatrix._41_42_43 - g_cameraPosition) < 0.f;
            
            if (frontFacing)
            {
                // Emit vertices
                for (int i = 0; i < vertsPerInstance; i++)
                {
                    VertexType output;
                    output.worldPosition = bbVertices[i] * float3(g_cardWidth, 1, g_cardHeight); // scale the position by the card scale
                    output.worldPosition = mul(float4(output.worldPosition, 1), worldMatrix).xyz;
                    output.position = mul(float4(output.worldPosition, 1), g_viewProj);
            
                    output.tex = bbTexCoords[i];
                    // Resolve sub-UVs
                    output.tex.x = lerp(instance.TexcoordUAndVRange.x,
                    instance.TexcoordUAndVRange.y,
                    output.tex.x);
                    output.tex.y = lerp(instance.TexcoordUAndVRange.z,
                    instance.TexcoordUAndVRange.w,
                    output.tex.y);

                    output.normal = rotatedFrontNormal;
        
                    outputVerts[i] = output;
                }
            
                // Emit indices
                for (int j = 0; j < trianglesPerInstance; j++)
                {
                    outputTris[j] = bbIndices[j];
                }
            }
            else
            {
                // Emit vertices
                for (int i = 0; i < vertsPerInstance; i++)
                {
                    VertexType output;
                    output.worldPosition = bbVertices[i] * float3(g_cardWidth, 1, g_cardHeight); // scale the position by the card scale
                    output.worldPosition = mul(float4(output.worldPosition, 1), worldMatrix).xyz;
                    output.position = mul(float4(output.worldPosition, 1), g_viewProj);
            
                    output.tex = bbTexCoords[i];
                    // Resolve sub-UVs, flipped laterally
                    output.tex.x = lerp(instance.TexcoordUAndVRange.x,
                                instance.TexcoordUAndVRange.y,
                                1 - output.tex.x);
                    output.tex.y = lerp(instance.TexcoordUAndVRange.z,
                                instance.TexcoordUAndVRange.w,
                                output.tex.y);
            
                    output.normal = -rotatedFrontNormal;
        
                    outputVerts[i] = output;
                }
            
                // Emit indices
                for (int j = 0; j < trianglesPerInstance; j++)
                {
                    outputTris[j] = bbIndices[j].xzy; // change the winding order for the back indices
                }
            }
        }
    }
    else
    {
        visible = false;
    }
    
    // Set the output count based on the visible instances 
    // This must be called by every thread, even if its 
    // instance is culled
    uint numInstancesEmitted = WaveActiveCountBits(visible);
    
    SetMeshOutputCounts(vertsPerInstance * numInstancesEmitted,
                        trianglesPerInstance * numInstancesEmitted);
    
    if (visible && instanceIndex < g_numInstances)
    {
        // Copy the data to the correct output index            
        uint index = WavePrefixCountBits(visible);
        
        for (int i = 0; i < vertsPerInstance; i++)
        {
            verts[index * vertsPerInstance + i] = outputVerts[i];
        }
        for (int j = 0; j < trianglesPerInstance; j++)
        {
            // Indices need to be offset by the vertices 
            // emitted before
            tris[index * trianglesPerInstance + j] = outputTris[j] + (vertsPerInstance * uint3(index, index, index));
        }
    }
}