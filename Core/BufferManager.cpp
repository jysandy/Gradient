#include "pch.h"

#include "Core/BufferManager.h"

#include <directxtk12/BufferHelpers.h>
#include <directxtk12/ResourceUploadBatch.h>

namespace Gradient
{
    std::unique_ptr<BufferManager> BufferManager::s_instance;

    void BufferManager::Initialize()
    {
        s_instance = std::make_unique<BufferManager>();
    }

    void BufferManager::Shutdown()
    {
        s_instance.reset();
    }

    BufferManager* BufferManager::Get()
    {
        return s_instance.get();
    }

    BufferManager::InstanceBufferHandle BufferManager::CreateInstanceBuffer(
        ID3D12Device* device,
        ID3D12CommandQueue* cq,
        const std::vector<InstanceData>& instanceData)
    {
        std::vector<InstanceData> transposedInstanceData;

        for (const auto& instance : instanceData)
        {
            transposedInstanceData.push_back({
                DirectX::XMMatrixTranspose(instance.World),
                instance.TexcoordURange,
                instance.TexcoordVRange
                });
        }

        auto handle = m_instanceBuffers.Allocate({
            BarrierResource(),
            static_cast<uint32_t>(transposedInstanceData.size())
            });

        auto entry = m_instanceBuffers.Get(handle);

        DirectX::ResourceUploadBatch uploadBatch(device);

        uploadBatch.Begin();

        // TODO: set flag to allow access as a UAV?
        DX::ThrowIfFailed(
            DirectX::CreateStaticBuffer(device,
                uploadBatch,
                transposedInstanceData.data(),
                transposedInstanceData.size(),
                D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
                entry->Resource.ReleaseAndGetAddressOf()));
        entry->Resource.SetState(D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);

        auto uploadFinished = uploadBatch.End(cq);
        uploadFinished.wait();

        return handle;
    }

    BufferManager::InstanceBufferEntry* BufferManager::GetInstanceBuffer(InstanceBufferHandle handle)
    {
        return m_instanceBuffers.Get(handle);
    }

    BufferManager::MeshHandle BufferManager::AddMesh(Rendering::ProceduralMesh&& mesh)
    {
        return m_meshes.Allocate(mesh);
    }

    void BufferManager::RemoveMesh(MeshHandle handle)
    {
        m_meshes.Remove(handle);
    }

    Rendering::ProceduralMesh* BufferManager::GetMesh(BufferManager::MeshHandle handle)
    {
        return m_meshes.Get(handle);
    }

#pragma region Mesh creation

    BufferManager::MeshHandle BufferManager::CreateMeshFromVertices(
        ID3D12Device* device,
        ID3D12CommandQueue* cq,
        const Rendering::ProceduralMesh::VertexCollection& vertices,
        const Rendering::ProceduralMesh::IndexCollection& indices
    )
    {
        return AddMesh(Rendering::ProceduralMesh::CreateFromVertices(
            device, cq, vertices, indices
        ));
    }

    BufferManager::MeshHandle BufferManager::CreateBox(
        ID3D12Device* device,
        ID3D12CommandQueue* cq,
        const DirectX::XMFLOAT3& size,
        bool rhcoords,
        bool invertn)
    {
        return AddMesh(Rendering::ProceduralMesh::CreateBox(
            device, cq, size, rhcoords, invertn
        ));
    }

    BufferManager::MeshHandle BufferManager::CreateSphere(
        ID3D12Device* device,
        ID3D12CommandQueue* cq,
        float diameter,
        size_t tessellation,
        bool rhcoords,
        bool invertn)
    {
        return AddMesh(Rendering::ProceduralMesh::CreateSphere(
            device, cq, diameter, tessellation, rhcoords, invertn
        ));
    }

    BufferManager::MeshHandle BufferManager::CreateGeoSphere(
        ID3D12Device* device,
        ID3D12CommandQueue* cq,
        float diameter,
        size_t tessellation,
        bool rhcoords)
    {
        return AddMesh(Rendering::ProceduralMesh::CreateGeoSphere(
            device, cq, diameter, tessellation, rhcoords
        ));
    }

    BufferManager::MeshHandle BufferManager::CreateGrid(
        ID3D12Device* device,
        ID3D12CommandQueue* cq,
        const float& width,
        const float& height,
        const float& divisions,
        bool tiled)
    {
        return AddMesh(Rendering::ProceduralMesh::CreateGrid(
            device, cq, width, height, divisions, tiled
        ));
    }

    BufferManager::MeshHandle BufferManager::CreateBillboard(
        ID3D12Device* device,
        ID3D12CommandQueue* cq,
        const float& width,
        const float& height)
    {
        return AddMesh(Rendering::ProceduralMesh::CreateBillboard(
            device, cq, width, height
        ));
    }

    BufferManager::MeshHandle BufferManager::CreateFrustum(
        ID3D12Device* device,
        ID3D12CommandQueue* cq,
        const float& topRadius,
        const float& bottomRadius,
        const float& height)
    {
        return AddMesh(Rendering::ProceduralMesh::CreateFrustum(
            device, cq, topRadius, bottomRadius, height
        ));
    }

    BufferManager::MeshHandle BufferManager::CreateAngledFrustum(
        ID3D12Device* device,
        ID3D12CommandQueue* cq,
        const float& bottomRadius,
        const float& topRadius,
        const DirectX::SimpleMath::Vector3& topCentre,
        const DirectX::SimpleMath::Quaternion& topRotation)
    {
        return AddMesh(Rendering::ProceduralMesh::CreateAngledFrustum(
            device, cq, bottomRadius, topRadius, topCentre,topRotation
        ));
    }

    BufferManager::MeshHandle BufferManager::CreateFromPart(
        ID3D12Device* device,
        ID3D12CommandQueue* cq,
        const Rendering::ProceduralMesh::MeshPart& part)
    {
        return AddMesh(Rendering::ProceduralMesh::CreateFromPart(
            device, cq, part
        ));
    }

#pragma endregion
}