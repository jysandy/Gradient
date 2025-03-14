#pragma once

#include "pch.h"
#include <memory>
#include "Core/Rendering/IDrawable.h"
#include <directxtk12/VertexTypes.h>
#include <directxtk12/SimpleMath.h>

namespace Gradient::Rendering
{
    class GeometricPrimitive : public IDrawable
    {
    public:
        using VertexType = DirectX::VertexPositionNormalTexture;
        using VertexCollection = std::vector<DirectX::VertexPositionNormalTexture>;
        using IndexCollection = std::vector<uint16_t>;


        virtual ~GeometricPrimitive() = default;

        virtual void Draw(ID3D12GraphicsCommandList* cl) override;

        static std::unique_ptr<GeometricPrimitive> CreateBox(
            ID3D12Device* device,
            ID3D12CommandQueue* cq,
            const DirectX::XMFLOAT3& size,
            bool rhcoords = true,
            bool invertn = false);

        static std::unique_ptr<GeometricPrimitive> CreateSphere(
            ID3D12Device* device,
            ID3D12CommandQueue* cq,
            float diameter = 1,
            size_t tessellation = 16,
            bool rhcoords = true,
            bool invertn = false);

        static std::unique_ptr<GeometricPrimitive> CreateGeoSphere(
            ID3D12Device* device,
            ID3D12CommandQueue* cq,
            float diameter = 1,
            size_t tessellation = 3,
            bool rhcoords = true);

        static std::unique_ptr<GeometricPrimitive> CreateGrid(
            ID3D12Device* device,
            ID3D12CommandQueue* cq,
            const float& width = 10,
            const float& height = 10,
            const float& divisions = 10,
            bool tiled = true);

        static std::unique_ptr<GeometricPrimitive> CreateFrustum(
            ID3D12Device* device,
            ID3D12CommandQueue* cq,
            const float& topRadius = 1,
            const float& bottomRadius = 1,
            const float& height = 3);

    private:
        GeometricPrimitive() = default;

        void Initialize(ID3D12Device* device,
            ID3D12CommandQueue* cq,
            VertexCollection vertices,
            IndexCollection indices);

        Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;
        UINT m_vertexCount;
        UINT m_indexCount;

        D3D12_VERTEX_BUFFER_VIEW m_vbv;
        D3D12_INDEX_BUFFER_VIEW m_ibv;
    };
}