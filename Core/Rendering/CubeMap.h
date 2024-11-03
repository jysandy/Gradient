#pragma once

#include "pch.h"
#include <directxtk/SimpleMath.h>
#include <functional>

namespace Gradient::Rendering
{
    class CubeMap
    {
    public:
        using DrawFn = std::function<void(DirectX::SimpleMath::Matrix, DirectX::SimpleMath::Matrix)>;

        CubeMap(ID3D11Device* device,
            int width,
            DXGI_FORMAT format);

        void Render(ID3D11DeviceContext* context, DrawFn fn);
        ID3D11ShaderResourceView* GetSRV() const;

    private:
        Microsoft::WRL::ComPtr<ID3D11Texture2D> m_texture;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_srv;
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_rtv[6];
        Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_dsv;

        D3D11_VIEWPORT m_viewport;
    };
}