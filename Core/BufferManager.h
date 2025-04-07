#pragma once

#include "pch.h"

#include "Core/BarrierResource.h"
#include "Core/FreeListAllocator.h"
#include "Core/Rendering/ProceduralMesh.h"

#include <optional>

namespace Gradient
{
    class BufferManager
    {
    public:
        struct __declspec(align(16)) InstanceData
        {
            DirectX::XMMATRIX World;
            DirectX::XMFLOAT2 TexcoordURange;
            DirectX::XMFLOAT2 TexcoordVRange;
        };

        struct InstanceBufferEntry
        {
            BarrierResource Resource;
            uint32_t InstanceCount;
        };

        using InstanceBufferList = FreeListAllocator<InstanceBufferEntry>;
        using InstanceBufferHandle = InstanceBufferList::Handle;

        using MeshList = FreeListAllocator<Rendering::ProceduralMesh>;
        using MeshHandle = MeshList::Handle;

        static void Initialize();
        static void Shutdown();
        static BufferManager* Get();

        InstanceBufferHandle CreateInstanceBuffer(ID3D12Device* device,
            ID3D12CommandQueue* cq,
            const std::vector<InstanceData>& instanceData);
        InstanceBufferEntry* GetInstanceBuffer(InstanceBufferHandle handle);

        MeshHandle AddMesh(Rendering::ProceduralMesh&& mesh);
        void RemoveMesh(MeshHandle handle);
        Rendering::ProceduralMesh* GetMesh(MeshHandle handle);

#pragma region Mesh creation
        
        MeshHandle CreateMeshFromVertices(
            ID3D12Device* device,
            ID3D12CommandQueue* cq,
            const Rendering::ProceduralMesh::VertexCollection& vertices,
            const Rendering::ProceduralMesh::IndexCollection& indices
        );

        MeshHandle CreateBox(
            ID3D12Device* device,
            ID3D12CommandQueue* cq,
            const DirectX::XMFLOAT3& size,
            bool rhcoords = true,
            bool invertn = false);

        MeshHandle CreateSphere(
            ID3D12Device* device,
            ID3D12CommandQueue* cq,
            float diameter = 1,
            size_t tessellation = 16,
            bool rhcoords = true,
            bool invertn = false);

        MeshHandle CreateGeoSphere(
            ID3D12Device* device,
            ID3D12CommandQueue* cq,
            float diameter = 1,
            size_t tessellation = 3,
            bool rhcoords = true);

        MeshHandle CreateGrid(
            ID3D12Device* device,
            ID3D12CommandQueue* cq,
            const float& width = 10,
            const float& height = 10,
            const float& divisions = 10,
            bool tiled = true);

        MeshHandle CreateBillboard(
            ID3D12Device* device,
            ID3D12CommandQueue* cq,
            const float& width = 1,
            const float& height = 1);

        MeshHandle CreateFrustum(
            ID3D12Device* device,
            ID3D12CommandQueue* cq,
            const float& topRadius = 1,
            const float& bottomRadius = 1,
            const float& height = 3);

        MeshHandle CreateAngledFrustum(
            ID3D12Device* device,
            ID3D12CommandQueue* cq,
            const float& bottomRadius = 1,
            const float& topRadius = 1,
            const DirectX::SimpleMath::Vector3& topCentre = { 0, 3, 0 },
            const DirectX::SimpleMath::Quaternion& topRotation = DirectX::SimpleMath::Quaternion::Identity);

        MeshHandle CreateFromPart(
            ID3D12Device* device,
            ID3D12CommandQueue* cq,
            const Rendering::ProceduralMesh::MeshPart& part
        );

#pragma endregion

    private:
        static std::unique_ptr<BufferManager> s_instance;

        InstanceBufferList m_instanceBuffers;
        MeshList m_meshes;
    };
}