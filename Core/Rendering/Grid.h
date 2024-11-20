#pragma once

#include "pch.h"
#include <memory>
#include "Core/Rendering/IDrawable.h"
#include <directxtk/VertexTypes.h>
#include <directxtk/GeometricPrimitive.h>

namespace Gradient::Rendering
{
    class Grid : public IDrawable
    {
    public:
        using VertexType = DirectX::VertexPositionNormalTexture;

        Grid(ID3D11Device* device, ID3D11DeviceContext* context);
        virtual ~Grid() = default;
        
        virtual void Draw(ID3D11DeviceContext* context) override;

    private:
        Microsoft::WRL::ComPtr<ID3D11Buffer> m_vertexBuffer;
        Microsoft::WRL::ComPtr<ID3D11Buffer> m_indexBuffer;
        UINT m_indexCount;
    };
}