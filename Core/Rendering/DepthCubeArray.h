#pragma once

#include "pch.h"
#include <directxtk/SimpleMath.h>
#include <functional>

namespace Gradient::Rendering
{
    class DepthCubeArray
    {
    public:
        using DrawFn = std::function<void(DirectX::SimpleMath::Matrix, DirectX::SimpleMath::Matrix)>;

        DepthCubeArray(ID3D11Device* device,
            int width,
            int numCubes);

        void Render(ID3D11DeviceContext* context,
            int cubeMapIndex,
            DirectX::SimpleMath::Vector3 origin,
            float nearPlane,
            float farPlane,
            DrawFn fn);

        ID3D11ShaderResourceView* GetSRV() const;

    private:
        Microsoft::WRL::ComPtr<ID3D11Texture2D> m_cubemapArray;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_srv;
        std::vector<Microsoft::WRL::ComPtr<ID3D11DepthStencilView>> m_dsvs;

        D3D11_VIEWPORT m_viewport;
    };
}