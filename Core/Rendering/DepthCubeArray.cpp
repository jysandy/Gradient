#include "pch.h"

#include "Core/Rendering/DepthCubeArray.h"

namespace Gradient::Rendering
{
    DepthCubeArray::DepthCubeArray(ID3D11Device* device,
        int width,
        int numCubes)
    {
        m_viewport.TopLeftX = 0.f;
        m_viewport.TopLeftY = 0.f;
        m_viewport.Width = (float)width;
        m_viewport.Height = (float)width;
        m_viewport.MinDepth = 0.f;
        m_viewport.MaxDepth = 1.f;

        CD3D11_TEXTURE2D_DESC depthStencilDesc(
            DXGI_FORMAT_R32_TYPELESS,
            (int)width,
            (int)width,
            6 * numCubes,
            1, // Use a single mipmap level.
            D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE
        );
        depthStencilDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

        DX::ThrowIfFailed(device->CreateTexture2D(
            &depthStencilDesc,
            nullptr,
            m_cubemapArray.ReleaseAndGetAddressOf()
        ));

        CD3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc(
            D3D11_DSV_DIMENSION_TEXTURE2DARRAY,
            DXGI_FORMAT_D32_FLOAT
        );

        dsvDesc.Texture2DArray.ArraySize = 1;

        for (int cubeMapIndex = 0; cubeMapIndex < numCubes; cubeMapIndex++)
        {
            for (int i = 0; i < 6; i++)
            {
                dsvDesc.Texture2DArray.FirstArraySlice = cubeMapIndex * 6 + i;

                Microsoft::WRL::ComPtr<ID3D11DepthStencilView> dsv;
                DX::ThrowIfFailed(device->CreateDepthStencilView(
                    m_cubemapArray.Get(),
                    &dsvDesc,
                    dsv.ReleaseAndGetAddressOf()
                ));
                m_dsvs.push_back(dsv);
            }
        }

        auto srvDesc = CD3D11_SHADER_RESOURCE_VIEW_DESC();
        srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
        srvDesc.TextureCubeArray.MostDetailedMip = 0;
        srvDesc.TextureCubeArray.MipLevels = 1;
        srvDesc.TextureCubeArray.First2DArrayFace = 0;
        srvDesc.TextureCubeArray.NumCubes = numCubes;

        DX::ThrowIfFailed(device->CreateShaderResourceView(
            m_cubemapArray.Get(), &srvDesc, m_srv.ReleaseAndGetAddressOf()));
    }

    void DepthCubeArray::Render(ID3D11DeviceContext* context,
        int cubeMapIndex,
        DirectX::SimpleMath::Vector3 origin,
        float nearPlane,
        float farPlane,
        DrawFn fn)
    {
        using namespace DirectX::SimpleMath;
        context->RSSetViewports(1, &m_viewport);

        // +X
        // -X
        // +Y
        // -Y
        // +Z
        // -Z
        Vector3 lookAt[6] = {
            Vector3::UnitX,
            -Vector3::UnitX,
            Vector3::UnitY,
            -Vector3::UnitY,
            // We look down -UnitZ instead of UnitZ 
            // since DirectX seems to expect us to 
            // work with left handed coordinates.
            -Vector3::UnitZ,
            Vector3::UnitZ
        };
        Vector3 up[6] = {
           Vector3::UnitY,
           Vector3::UnitY,
           Vector3::UnitZ,
           -Vector3::UnitZ,
           Vector3::UnitY,
           Vector3::UnitY
        };

        auto projectionMatrix = Matrix::CreatePerspectiveFieldOfView(
            DirectX::XM_PIDIV2,
            1.f, nearPlane, farPlane);

        for (int i = 0; i < 6; i++)
        {
            auto dsvIndex = cubeMapIndex * 6 + i;

            context->ClearDepthStencilView(m_dsvs[dsvIndex].Get(), 
                D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
            context->OMSetRenderTargets(0, nullptr, m_dsvs[dsvIndex].Get());

            auto look = lookAt[i];

            auto viewMatrix = Matrix::CreateLookAt(origin,
                origin + look, up[i]);
            fn(viewMatrix, projectionMatrix);
        }
    }

    ID3D11ShaderResourceView* DepthCubeArray::GetSRV() const
    {
        return m_srv.Get();
    }
}