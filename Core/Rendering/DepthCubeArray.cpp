#include "pch.h"

#include "Core/Rendering/DepthCubeArray.h"

namespace Gradient::Rendering
{
    DepthCubeArray::DepthCubeArray(ID3D12Device* device,
        int width,
        int numCubes)
    {
        m_viewport.TopLeftX = 0.f;
        m_viewport.TopLeftY = 0.f;
        m_viewport.Width = (float)width;
        m_viewport.Height = (float)width;
        m_viewport.MinDepth = 0.f;
        m_viewport.MaxDepth = 1.f;

        auto depthStencilDesc = CD3DX12_RESOURCE_DESC::Tex2D(
          DXGI_FORMAT_R32_TYPELESS,
            width,
            width,
            6 * numCubes,
            1
        );
        depthStencilDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        D3D12_CLEAR_VALUE depthClearValue = {};
        depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;
        depthClearValue.DepthStencil.Depth = 1.0f;
        depthClearValue.DepthStencil.Stencil = 0;

        m_cubemapArray.Create(device,
            &depthStencilDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &depthClearValue);

        auto dsvDesc = D3D12_DEPTH_STENCIL_VIEW_DESC();
        dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;

        dsvDesc.Texture2DArray.ArraySize = 1;

        auto gmm = GraphicsMemoryManager::Get();
        for (int cubeMapIndex = 0; cubeMapIndex < numCubes; cubeMapIndex++)
        {
            for (int i = 0; i < 6; i++)
            {
                dsvDesc.Texture2DArray.FirstArraySlice = cubeMapIndex * 6 + i;

                m_dsvs.push_back(gmm->CreateDSV(device,
                    m_cubemapArray.Get(),
                    dsvDesc));
            }
        }

        auto srvDesc = D3D12_SHADER_RESOURCE_VIEW_DESC();
        srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
        srvDesc.TextureCubeArray.MostDetailedMip = 0;
        srvDesc.TextureCubeArray.MipLevels = 1;
        srvDesc.TextureCubeArray.First2DArrayFace = 0;
        srvDesc.TextureCubeArray.NumCubes = numCubes;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        m_srv = gmm->CreateSRV(device,
            m_cubemapArray.Get(),
            &srvDesc);
    }

    void DepthCubeArray::Render(ID3D12GraphicsCommandList* cl,
        int cubeMapIndex,
        DirectX::SimpleMath::Vector3 origin,
        float nearPlane,
        float farPlane,
        DrawFn fn)
    {
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

        m_cubemapArray.Transition(cl, D3D12_RESOURCE_STATE_DEPTH_WRITE);

        auto projectionMatrix = Matrix::CreatePerspectiveFieldOfView(
            DirectX::XM_PIDIV2,
            1.f, nearPlane, farPlane);

        auto gmm = GraphicsMemoryManager::Get();

        for (int i = 0; i < 6; i++)
        {
            auto dsvIndex = cubeMapIndex * 6 + i;

            auto dsvHandle = m_dsvs[dsvIndex]->GetCPUHandle();

            cl->ClearDepthStencilView(dsvHandle,
                D3D12_CLEAR_FLAG_DEPTH,
                1.0f,
                0, 0, nullptr);

            cl->OMSetRenderTargets(0, nullptr, FALSE, &dsvHandle);

            auto look = lookAt[i];

            auto viewMatrix = Matrix::CreateLookAt(origin,
                origin + look, up[i]);
            fn(viewMatrix, projectionMatrix, look);
        }
    }

    GraphicsMemoryManager::DescriptorView DepthCubeArray::GetSRV() const
    {
        return m_srv;
    }

    void DepthCubeArray::TransitionToShaderResource(ID3D12GraphicsCommandList* cl)
    {
        m_cubemapArray.Transition(cl, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    }
}