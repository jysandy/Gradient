#pragma once

#include "pch.h"

#include "Core/BarrierResource.h"
#include "Core/FreeListAllocator.h"
#include "Core/Rendering/ProceduralMesh.h"

#include <directxtk12/BufferHelpers.h>
#include <directxtk12/ResourceUploadBatch.h>
#include <optional>

namespace Gradient
{
    class BufferManager
    {
    public:

        struct __declspec(align(16)) InstanceData
        {
            // Not supporting scale to save space
            DirectX::XMFLOAT3 Position;
            float pad;
            DirectX::XMFLOAT4 RotationQuat;
            DirectX::XMFLOAT2 TexcoordURange;
            DirectX::XMFLOAT2 TexcoordVRange;
        };

        struct StructuredBufferEntry
        {
            BarrierResource Resource;
            uint32_t Count;
        };

        using StructuredBufferList = FreeListAllocator<StructuredBufferEntry>;
        using StructuredBufferHandle = StructuredBufferList::Handle;

        using MeshList = FreeListAllocator<Rendering::ProceduralMesh>;
        using MeshHandle = MeshList::Handle;

        static void Initialize();
        static void Shutdown();
        static BufferManager* Get();

        template <typename T>
        StructuredBufferHandle CreateStructuredBuffer(ID3D12Device* device,
            ID3D12CommandQueue* cq,
            const std::vector<T>& bufferData);

        StructuredBufferEntry* GetStructuredBuffer(StructuredBufferHandle handle);

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
            const Rendering::ProceduralMesh::MeshPart& part,
            float simplificationRate = 0.f,
            float errorRate = 0.1f
        );

#pragma endregion

    private:
        static std::unique_ptr<BufferManager> s_instance;

        StructuredBufferList m_structuredBuffers;
        MeshList m_meshes;
    };

    template <typename T>
    BufferManager::StructuredBufferHandle BufferManager::CreateStructuredBuffer(
        ID3D12Device* device,
        ID3D12CommandQueue* cq,
        const std::vector<T>& instanceData)
    {
        auto handle = m_structuredBuffers.Allocate({
            BarrierResource(),
            static_cast<uint32_t>(instanceData.size())
            });

        auto entry = m_structuredBuffers.Get(handle);

        DirectX::ResourceUploadBatch uploadBatch(device);

        uploadBatch.Begin();

        DX::ThrowIfFailed(
            DirectX::CreateStaticBuffer(device,
                uploadBatch,
                instanceData.data(),
                instanceData.size(),
                D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
                entry->Resource.ReleaseAndGetAddressOf(),
                D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS));
        entry->Resource.SetState(D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);

        auto uploadFinished = uploadBatch.End(cq);
        uploadFinished.wait();

        return handle;
    }
}