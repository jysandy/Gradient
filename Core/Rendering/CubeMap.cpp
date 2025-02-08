#include "pch.h"

#include "Core/Rendering/CubeMap.h"

#include <directxtk12/DirectXHelpers.h>

namespace Gradient::Rendering
{
    CubeMap::CubeMap(ID3D12Device* device,
        int width,
        DXGI_FORMAT format)
    {
        auto gmm = GraphicsMemoryManager::Get();

        m_viewport.TopLeftX = 0.f;
        m_viewport.TopLeftY = 0.f;
        m_viewport.Width = (float)width;
        m_viewport.Height = (float)width;
        m_viewport.MinDepth = 0.f;
        m_viewport.MaxDepth = 1.f;

        auto texDesc = CD3DX12_RESOURCE_DESC::Tex2D(format,
            width,
            width,
            6);
        texDesc.Flags
            |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

        D3D12_CLEAR_VALUE clearValue = {};
        clearValue.Format = format;
        clearValue.Color[0] = DirectX::ColorsLinear::CornflowerBlue.f[0];
        clearValue.Color[1] = DirectX::ColorsLinear::CornflowerBlue.f[1];
        clearValue.Color[2] = DirectX::ColorsLinear::CornflowerBlue.f[2];
        clearValue.Color[3] = DirectX::ColorsLinear::CornflowerBlue.f[3];

        m_texture.Create(device,
            &texDesc,
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            &clearValue);

        auto rtvDesc = D3D12_RENDER_TARGET_VIEW_DESC();
        rtvDesc.Format = format;
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
        rtvDesc.Texture2DArray.ArraySize = 1;

        for (int i = 0; i < 6; i++)
        {
            rtvDesc.Texture2DArray.FirstArraySlice = i;
            m_rtv[i] = gmm->CreateRTV(device, rtvDesc, m_texture.Get());
        }

        auto srvDesc = D3D12_SHADER_RESOURCE_VIEW_DESC();
        srvDesc.Format = format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
        srvDesc.TextureCube.MostDetailedMip = 0;
        srvDesc.TextureCube.MipLevels = 1;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        m_srv = gmm->CreateSRV(device,
            m_texture.Get(),
            &srvDesc);

        auto depthTexDesc = CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_D32_FLOAT,
            width,
            width
        );
        depthTexDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        D3D12_CLEAR_VALUE depthClearValue = {};
        depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;
        depthClearValue.DepthStencil.Depth = 1.0f;
        depthClearValue.DepthStencil.Stencil = 0;

        m_depthTex.Create(device,
            &depthTexDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &depthClearValue);

        auto dsvDesc = D3D12_DEPTH_STENCIL_VIEW_DESC();
        dsvDesc.Format = depthTexDesc.Format;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        m_dsv = gmm->CreateDSV(device, m_depthTex.Get(), dsvDesc);
    }

    void CubeMap::Render(ID3D12GraphicsCommandList* cl, DrawFn fn)
    {
        // TODO: Accept the cubemap origin as a parameter
        using namespace DirectX::SimpleMath;
        cl->RSSetViewports(1, &m_viewport);

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
            // TODO: Instead of this, change the up vectors to 
            // -Y, as explained by ChatGPT.
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

        m_texture.Transition(cl, D3D12_RESOURCE_STATE_RENDER_TARGET);

        // TODO: Accept the near and far planes as parameters
        auto projectionMatrix = Matrix::CreatePerspectiveFieldOfView(
            DirectX::XM_PIDIV2,
            1.f, 0.1f, 300.f);

        auto gmm = GraphicsMemoryManager::Get();

        for (int i = 0; i < 6; i++)
        {
            auto rtvHandle = m_rtv[i]->GetCPUHandle();
            auto dsvHandle = m_dsv->GetCPUHandle();

            cl->ClearRenderTargetView(rtvHandle,
                DirectX::ColorsLinear::CornflowerBlue,
                0, nullptr
            );

            cl->ClearDepthStencilView(dsvHandle,
                D3D12_CLEAR_FLAG_DEPTH,
                1.0f,
                0, 0, nullptr);

            cl->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

            auto viewMatrix = Matrix::CreateLookAt(Vector3::Zero,
                lookAt[i], up[i]);
            fn(viewMatrix, projectionMatrix);
        }
    }

    void CubeMap::TransitionToShaderResource(ID3D12GraphicsCommandList* cl)
    {
        m_texture.Transition(cl, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    }

    GraphicsMemoryManager::DescriptorView CubeMap::GetSRV() const
    {
        return m_srv;
    }
}