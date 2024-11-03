#include "pch.h"

#include "Core/Rendering/CubeMap.h"

namespace Gradient::Rendering
{
    CubeMap::CubeMap(ID3D11Device* device,
        int width,
        DXGI_FORMAT format)
    {
        m_viewport.TopLeftX = 0.f;
        m_viewport.TopLeftY = 0.f;
        m_viewport.Width = (float)width;
        m_viewport.Height = (float)width;
        m_viewport.MinDepth = 0.f;
        m_viewport.MaxDepth = 1.f;

        auto texDesc = CD3D11_TEXTURE2D_DESC();
        texDesc.Width = width;
        texDesc.Height = width;
        texDesc.MipLevels = 0;
        texDesc.ArraySize = 6;
        texDesc.SampleDesc.Count = 1;
        texDesc.SampleDesc.Quality = 0;
        texDesc.Format = format;
        texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        texDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS | D3D11_RESOURCE_MISC_TEXTURECUBE;

        DX::ThrowIfFailed(device->CreateTexture2D(&texDesc, nullptr,
            m_texture.ReleaseAndGetAddressOf()));

        auto rtvDesc = CD3D11_RENDER_TARGET_VIEW_DESC();
        rtvDesc.Format = format;
        rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
        rtvDesc.Texture2DArray.ArraySize = 1;

        for (int i = 0; i < 6; i++)
        {
            rtvDesc.Texture2DArray.FirstArraySlice = i;
            DX::ThrowIfFailed(device->CreateRenderTargetView(
                m_texture.Get(), &rtvDesc, m_rtv[i].ReleaseAndGetAddressOf()));
        }

        auto srvDesc = CD3D11_SHADER_RESOURCE_VIEW_DESC();
        srvDesc.Format = format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
        srvDesc.TextureCube.MostDetailedMip = 0;
        srvDesc.TextureCube.MipLevels = -1;

        DX::ThrowIfFailed(device->CreateShaderResourceView(
            m_texture.Get(), &srvDesc, m_srv.ReleaseAndGetAddressOf()));

        auto depthTexDesc = CD3D11_TEXTURE2D_DESC();
        depthTexDesc.Width = width;
        depthTexDesc.Height = width;
        depthTexDesc.MipLevels = 1;
        depthTexDesc.ArraySize = 1;
        depthTexDesc.SampleDesc.Count = 1;
        depthTexDesc.SampleDesc.Quality = 0;
        depthTexDesc.Format = DXGI_FORMAT_D32_FLOAT;
        depthTexDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

        Microsoft::WRL::ComPtr<ID3D11Texture2D> depthTex;
        DX::ThrowIfFailed(device->CreateTexture2D(
            &depthTexDesc, nullptr, depthTex.ReleaseAndGetAddressOf()));

        auto dsvDesc = CD3D11_DEPTH_STENCIL_VIEW_DESC();
        dsvDesc.Format = depthTexDesc.Format;
        dsvDesc.Flags = 0;
        dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Texture2D.MipSlice = 0;
        DX::ThrowIfFailed(device->CreateDepthStencilView(
            depthTex.Get(), &dsvDesc, m_dsv.ReleaseAndGetAddressOf()));
    }

    void CubeMap::Render(ID3D11DeviceContext* context, DrawFn fn)
    {
        // TODO: Accept the cubemap origin as a parameter
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

        // TODO: Accept the near and far planes as parameters
        auto projectionMatrix = Matrix::CreatePerspectiveFieldOfView(
            DirectX::XM_PIDIV2,
            1.f, 0.1f, 500.f);

        for (int i = 0; i < 6; i++)
        {
            context->ClearRenderTargetView(m_rtv[i].Get(), DirectX::ColorsLinear::CornflowerBlue);
            context->ClearDepthStencilView(m_dsv.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
            context->OMSetRenderTargets(1, m_rtv[i].GetAddressOf(), m_dsv.Get());

            auto viewMatrix = Matrix::CreateLookAt(Vector3::Zero,
                lookAt[i], up[i]);
            fn(viewMatrix, projectionMatrix);
        }

        context->GenerateMips(m_srv.Get());
    }

    ID3D11ShaderResourceView* CubeMap::GetSRV() const
    {
        return m_srv.Get();
    }
}