#include "pch.h"

#include <vector>
#include <cstdint>
#include "Core/Rendering/Grid.h"
#include <directxtk/SimpleMath.h>
#include <directxtk/BufferHelpers.h>

namespace Gradient::Rendering
{
    Grid::Grid(ID3D11Device* device, ID3D11DeviceContext* context) : m_context(context)
    {
        using namespace DirectX;
        using namespace DirectX::SimpleMath;

        static const std::vector<VertexType> s_vertexData =
        {
            { Vector3{ -0.5f, 0.f, -0.5f }, Vector3::UnitY, Vector2{0.f, 0.f}}, // top-left
            { Vector3{ -0.5f, 0.f,  0.5f }, Vector3::UnitY, Vector2{0.f, 1.f}}, // bottom-left
            { Vector3{  0.5f, 0.f,  0.5f }, Vector3::UnitY, Vector2{1.f, 1.f}}, // bottom-right
            { Vector3{  0.5f, 0.f, -0.5f }, Vector3::UnitY, Vector2{1.f, 0.f}}, // top-right
        };

        static const std::vector<uint16_t> s_indexData =
        {
            0, 3, 1,
            1, 3, 2
        };

        DX::ThrowIfFailed(
            CreateStaticBuffer(device, s_vertexData,
                D3D11_BIND_VERTEX_BUFFER, m_vertexBuffer.ReleaseAndGetAddressOf()));
        DX::ThrowIfFailed(
            CreateStaticBuffer(device, s_indexData,
                D3D11_BIND_INDEX_BUFFER, m_indexBuffer.ReleaseAndGetAddressOf()));
        m_indexCount = s_indexData.size();
    }

    void Grid::Draw(Effects::IEntityEffect* effect, std::function<void()> setCustomState)
    {
        constexpr UINT vertexStride = sizeof(VertexType);
        constexpr UINT vertexOffset = 0;

        m_context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &vertexStride, &vertexOffset);
        m_context->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
        
        assert(effect != nullptr);
        effect->Apply(m_context);

        if (setCustomState)
        {
            setCustomState();
        }

        m_context->DrawIndexed(m_indexCount, 0, 0);
    }
}