#pragma once

#include "pch.h"
#include <memory>
#include "Core/Rendering/IDrawable.h"
#include <directxtk12/VertexTypes.h>
#include <directxtk12/SimpleMath.h>

namespace Gradient::Rendering
{
    class ProceduralMesh : public IDrawable
    {
    public:
        using VertexType = DirectX::VertexPositionNormalTexture;
        using VertexCollection = std::vector<DirectX::VertexPositionNormalTexture>;
        using IndexCollection = std::vector<uint32_t>;
        using NarrowIndexCollection = std::vector<uint16_t>;


        virtual ~ProceduralMesh() = default;

        virtual void Draw(ID3D12GraphicsCommandList* cl, uint32_t numInstances=1) override;

        const DirectX::BoundingBox& GetBoundingBox() const;

        struct MeshPart
        {
            VertexCollection Vertices;
            IndexCollection Indices;

            MeshPart Append(const MeshPart& appendage,
                DirectX::SimpleMath::Vector3 translation,
                DirectX::SimpleMath::Quaternion rotation);

            void AppendInPlace(const MeshPart& appendage,
                DirectX::SimpleMath::Vector3 translation,
                DirectX::SimpleMath::Quaternion rotation);
        };

        static ProceduralMesh CreateBox(
            ID3D12Device* device,
            ID3D12CommandQueue* cq,
            const DirectX::XMFLOAT3& size,
            bool rhcoords = true,
            bool invertn = false);

        static ProceduralMesh CreateSphere(
            ID3D12Device* device,
            ID3D12CommandQueue* cq,
            float diameter = 1,
            size_t tessellation = 16,
            bool rhcoords = true,
            bool invertn = false);

        static ProceduralMesh CreateGeoSphere(
            ID3D12Device* device,
            ID3D12CommandQueue* cq,
            float diameter = 1,
            size_t tessellation = 3,
            bool rhcoords = true);

        static ProceduralMesh CreateGrid(
            ID3D12Device* device,
            ID3D12CommandQueue* cq,
            const float& width = 10,
            const float& height = 10,
            const float& divisions = 10,
            bool tiled = true);

        static ProceduralMesh CreateBillboard(
            ID3D12Device* device,
            ID3D12CommandQueue* cq,
            const float& width = 1,
            const float& height = 1);

        static ProceduralMesh CreateFrustum(
            ID3D12Device* device,
            ID3D12CommandQueue* cq,
            const float& topRadius = 1,
            const float& bottomRadius = 1,
            const float& height = 3);

        static MeshPart CreateAngledFrustumPart(
            float bottomRadius,
            float topRadius,
            DirectX::SimpleMath::Vector3 topCentre,
            DirectX::SimpleMath::Quaternion topRotation,
            int numVerticalSections
        );

        static ProceduralMesh CreateAngledFrustum(
            ID3D12Device* device,
            ID3D12CommandQueue* cq,
            const float& bottomRadius = 1,
            const float& topRadius = 1,
            const DirectX::SimpleMath::Vector3& topCentre = {0, 3, 0},
            const DirectX::SimpleMath::Quaternion& topRotation = DirectX::SimpleMath::Quaternion::Identity);

        static ProceduralMesh CreateFromVertices(
            ID3D12Device* device,
            ID3D12CommandQueue* cq,
            const VertexCollection& vertices,
            const IndexCollection& indices,
            float simplificationRate = 0.f,
            float errorRate = 0.1f
        );

        static ProceduralMesh CreateFromPart(
            ID3D12Device* device,
            ID3D12CommandQueue* cq,
            const MeshPart& part,
            float simplificationRate = 0.f,
            float errorRate = 0.1f
        );

    private:
        ProceduralMesh() = default;

        void Initialize(ID3D12Device* device,
            ID3D12CommandQueue* cq,
            const VertexCollection& vertices,
            const IndexCollection& indices,
            float simplificationRate = 0.f,
            float errorRate = 0.1f);

        Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;
        UINT m_vertexCount;
        UINT m_indexCount;

        D3D12_VERTEX_BUFFER_VIEW m_vbv;
        D3D12_INDEX_BUFFER_VIEW m_ibv;
        DirectX::BoundingBox m_boundingBox;
    };
}