#pragma once

#include "pch.h"
#include <memory>
#include "Core/Rendering/IDrawable.h"
#include <directxtk/VertexTypes.h>
#include <directxtk/SimpleMath.h>

namespace Gradient::Rendering
{
    class GeometricPrimitive : public IDrawable
    {
    public:
        using VertexType = DirectX::VertexPositionNormalTexture;
        using VertexCollection = std::vector<DirectX::VertexPositionNormalTexture>;
        using IndexCollection = std::vector<uint16_t>;


        virtual ~GeometricPrimitive() = default;

        virtual void Draw(Effects::IEntityEffect* effect, std::function<void()> setCustomState = nullptr) override;

        static std::unique_ptr<GeometricPrimitive> CreateBox(ID3D11Device* device,
            ID3D11DeviceContext* deviceContext,
            const DirectX::XMFLOAT3& size,
            bool rhcoords = true,
            bool invertn = false);

        static std::unique_ptr<GeometricPrimitive> CreateSphere(
            ID3D11Device* device,
            ID3D11DeviceContext* deviceContext,
            float diameter = 1,
            size_t tessellation = 16,
            bool rhcoords = true,
            bool invertn = false);

        std::unique_ptr<GeometricPrimitive> CreateGeoSphere(
            ID3D11Device* device,
            ID3D11DeviceContext* deviceContext,
            float diameter = 1,
            size_t tessellation = 3,
            bool rhcoords = true);

    private:
        GeometricPrimitive() = default;

        void Initialize(ID3D11Device* device, 
            ID3D11DeviceContext* context,
            VertexCollection vertices,
            IndexCollection indices);

        Microsoft::WRL::ComPtr<ID3D11Buffer> m_vertexBuffer;
        Microsoft::WRL::ComPtr<ID3D11Buffer> m_indexBuffer;
        UINT m_indexCount;

        // TODO: Remove the reference to the context
        ID3D11DeviceContext* m_context;

    };
}