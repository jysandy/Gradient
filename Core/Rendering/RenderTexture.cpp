#include "pch.h"

#include "Core/Rendering/RenderTexture.h"
#include <directxtk12/SimpleMath.h>
#include <directxtk12/DirectXHelpers.h>
#include <directxtk12/ResourceUploadBatch.h>

namespace
{
    constexpr UINT MSAA_COUNT = 4;
    constexpr UINT MSAA_QUALITY = 0;
}

namespace Gradient::Rendering
{
    RenderTexture::RenderTexture(
        ID3D12Device* device,
        UINT width,
        UINT height,
        DXGI_FORMAT format,
        bool multisamplingEnabled)
        : m_multisamplingEnabled(multisamplingEnabled),
        m_format(format)
    {
        const auto depthFormat = DXGI_FORMAT_D32_FLOAT;
        auto gmm = GraphicsMemoryManager::Get();

        m_size = RECT();
        m_size.left = 0;
        m_size.right = width;
        m_size.top = 0;
        m_size.bottom = height;

        D3D12_RESOURCE_DESC rtDesc = {};
        D3D12_RESOURCE_DESC dsDesc = {};
        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};

        if (multisamplingEnabled)
        {
            rtDesc = CD3DX12_RESOURCE_DESC::Tex2D(
                format,
                width,
                height,
                1,
                1,
                MSAA_COUNT,
                MSAA_QUALITY
            );
            rtDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

            auto singleRTDesc = CD3DX12_RESOURCE_DESC::Tex2D(
                format,
                width,
                height,
                1,
                1
            );

            m_singleSampledRT.Create(device,
                &singleRTDesc,
                D3D12_RESOURCE_STATE_RESOLVE_DEST,
                nullptr);


            dsDesc = CD3DX12_RESOURCE_DESC::Tex2D(
                DXGI_FORMAT_D32_FLOAT,
                width,
                height,
                1,
                1,
                MSAA_COUNT,
                MSAA_QUALITY
            );
            dsDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

            dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
            dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
        }
        else
        {
            rtDesc = CD3DX12_RESOURCE_DESC();

            rtDesc.Format = format;
            rtDesc.Width = width;
            rtDesc.Height = height;
            rtDesc.MipLevels = 1;
            rtDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            rtDesc.DepthOrArraySize = 1;
            rtDesc.SampleDesc.Count = 1;
            rtDesc.SampleDesc.Quality = 0;
            rtDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

            dsDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            dsDesc.Width = width;
            dsDesc.Height = height;
            dsDesc.DepthOrArraySize = 1;
            dsDesc.MipLevels = 1;
            dsDesc.Format = DXGI_FORMAT_D32_FLOAT;
            dsDesc.SampleDesc.Count = 1;
            dsDesc.SampleDesc.Quality = 0;
            dsDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

            dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
            dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        }

        D3D12_CLEAR_VALUE clearValue = {};
        clearValue.Format = format;
        clearValue.Color[0] = DirectX::ColorsLinear::CornflowerBlue.f[0];
        clearValue.Color[1] = DirectX::ColorsLinear::CornflowerBlue.f[1];
        clearValue.Color[2] = DirectX::ColorsLinear::CornflowerBlue.f[2];
        clearValue.Color[3] = DirectX::ColorsLinear::CornflowerBlue.f[3];

        m_offscreenRT.Create(
            device,
            &rtDesc,
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            &clearValue
        );

        m_rtv = gmm->CreateRTV(device, m_offscreenRT.Get());

        D3D12_CLEAR_VALUE depthClearValue = {};
        depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;
        depthClearValue.DepthStencil.Depth = 1.0f;
        depthClearValue.DepthStencil.Stencil = 0;

        m_depthBuffer.Create(
            device,
            &dsDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &depthClearValue
        );

        m_dsv = gmm->CreateDSV(device,
            m_depthBuffer.Get(),
            dsvDesc);

        if (multisamplingEnabled)
        {
            m_srv = gmm->CreateSRV(device, m_singleSampledRT.Get());
        }
        else
        {
            m_srv = gmm->CreateSRV(device, m_offscreenRT.Get());
        }
    }

    ID3D12Resource* RenderTexture::GetTexture()
    {
        return m_offscreenRT.Get();
    }

    BarrierResource* RenderTexture::GetBarrierResource()
    {
        return &m_offscreenRT;
    }

    GraphicsMemoryManager::DescriptorView RenderTexture::GetRTV()
    {
        return m_rtv;
    }

    ID3D12Resource* RenderTexture::GetSingleSampledTexture()
    {
        if (m_multisamplingEnabled)
        {
            return m_singleSampledRT.Get();
        }
        else
        {
            return m_offscreenRT.Get();
        }
    }

    BarrierResource* RenderTexture::GetSingleSampledBarrierResource()
    {
        if (m_multisamplingEnabled)
        {
            return &m_singleSampledRT;
        }
        else
        {
            return &m_offscreenRT;
        }
    }

    void RenderTexture::CopyToSingleSampled(ID3D12GraphicsCommandList* cl)
    {
        if (m_multisamplingEnabled)
        {
            // TODO: Batch these barrier transitions
            m_offscreenRT.Transition(cl, D3D12_RESOURCE_STATE_RESOLVE_SOURCE);
            m_singleSampledRT.Transition(cl, D3D12_RESOURCE_STATE_RESOLVE_DEST);
            cl->ResolveSubresource(m_singleSampledRT.Get(), 0,
                m_offscreenRT.Get(), 0,
                m_format);
        }
    }

    void RenderTexture::ClearAndSetAsTarget(ID3D12GraphicsCommandList* cl)
    {
        auto gmm = GraphicsMemoryManager::Get();

        // TODO: Batch these barrier transitions
        m_offscreenRT.Transition(cl, D3D12_RESOURCE_STATE_RENDER_TARGET);
        m_depthBuffer.Transition(cl, D3D12_RESOURCE_STATE_DEPTH_WRITE);

        auto rtvHandle = m_rtv->GetCPUHandle();
        auto dsvHandle = m_dsv->GetCPUHandle();

        cl->ClearRenderTargetView(rtvHandle,
            DirectX::ColorsLinear::CornflowerBlue,
            0, nullptr);

        cl->ClearDepthStencilView(dsvHandle,
            D3D12_CLEAR_FLAG_DEPTH,
            1.0f,
            0, 0, nullptr);

        cl->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
    }

    void RenderTexture::SetAsTarget(ID3D12GraphicsCommandList* cl)
    {
        auto gmm = GraphicsMemoryManager::Get();

        // TODO: Batch these barrier transitions
        m_offscreenRT.Transition(cl, D3D12_RESOURCE_STATE_RENDER_TARGET);
        m_depthBuffer.Transition(cl, D3D12_RESOURCE_STATE_DEPTH_WRITE);

        auto rtvHandle = m_rtv->GetCPUHandle();
        auto dsvHandle = m_dsv->GetCPUHandle();

        cl->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
    }

    GraphicsMemoryManager::DescriptorView RenderTexture::GetSRV()
    {
        return m_srv;
    }

    RECT RenderTexture::GetOutputSize()
    {
        return m_size;
    }

    void RenderTexture::DrawTo(
        ID3D12GraphicsCommandList* cl,
        Gradient::Rendering::RenderTexture* destination,
        TextureDrawer* texDrawer)
    {
        destination->ClearAndSetAsTarget(cl);
        auto outputSize = destination->GetOutputSize();
        
        // TODO: Batch this transition with the clearing?
        if (m_multisamplingEnabled)
        {
            // Multisampled RTs have to be resolved before they can be drawn.
            m_singleSampledRT.Transition(cl, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        }
        else
        {
            m_offscreenRT.Transition(cl, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        }

        texDrawer->Draw(cl, m_srv,
            m_size,
            outputSize);
    }
}