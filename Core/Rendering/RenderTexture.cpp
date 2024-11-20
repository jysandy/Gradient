#include "pch.h"

#include "Core/Rendering/RenderTexture.h"
#include <directxtk/SimpleMath.h>

namespace
{
    constexpr UINT MSAA_COUNT = 4;
    constexpr UINT MSAA_QUALITY = 0;
}

namespace Gradient::Rendering
{
    RenderTexture::RenderTexture(
        ID3D11Device* device,
        ID3D11DeviceContext* context,
        std::shared_ptr<DirectX::CommonStates> commonStates,
        UINT width,
        UINT height,
        DXGI_FORMAT format,
        bool multisamplingEnabled)
        : m_multisamplingEnabled(multisamplingEnabled),
        m_format(format),
        m_states(commonStates)
    {
        m_spriteBatch = std::make_unique<DirectX::SpriteBatch>(context);

        m_outputSize = RECT();
        m_outputSize.left = 0;
        m_outputSize.right = width;
        m_outputSize.top = 0;
        m_outputSize.bottom = height;

        CD3D11_TEXTURE2D_DESC rtDesc;
        CD3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
        CD3D11_TEXTURE2D_DESC dsDesc;
        CD3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;

        if (multisamplingEnabled)
        {
            rtDesc = CD3D11_TEXTURE2D_DESC(format,
                width, height, 1, 1,
                D3D11_BIND_RENDER_TARGET,
                D3D11_USAGE_DEFAULT, 0,
                MSAA_COUNT, MSAA_QUALITY);


            rtvDesc = CD3D11_RENDER_TARGET_VIEW_DESC(
                D3D11_RTV_DIMENSION_TEXTURE2DMS
            );

            CD3D11_TEXTURE2D_DESC singleRTDesc(format,
                width, height, 1, 1,
                D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
                D3D11_USAGE_DEFAULT);

            DX::ThrowIfFailed(
                device->CreateTexture2D(&singleRTDesc, nullptr,
                    m_singleSampledRT.ReleaseAndGetAddressOf()));

            dsDesc = CD3D11_TEXTURE2D_DESC(DXGI_FORMAT_D32_FLOAT,
                width, height, 1, 1,
                D3D11_BIND_DEPTH_STENCIL, D3D11_USAGE_DEFAULT, 0,
                MSAA_COUNT, MSAA_QUALITY);

            dsvDesc = CD3D11_DEPTH_STENCIL_VIEW_DESC(D3D11_DSV_DIMENSION_TEXTURE2DMS);
        }
        else
        {
            rtDesc = CD3D11_TEXTURE2D_DESC(format,
                width, height, 1, 1,
                D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
                D3D11_USAGE_DEFAULT);

            rtvDesc = CD3D11_RENDER_TARGET_VIEW_DESC(
                D3D11_RTV_DIMENSION_TEXTURE2D
            );

            dsDesc = CD3D11_TEXTURE2D_DESC(DXGI_FORMAT_D32_FLOAT,
                width, height, 1, 1,
                D3D11_BIND_DEPTH_STENCIL, D3D11_USAGE_DEFAULT);

            dsvDesc = CD3D11_DEPTH_STENCIL_VIEW_DESC(D3D11_DSV_DIMENSION_TEXTURE2D);
        }

        DX::ThrowIfFailed(
            device->CreateTexture2D(&rtDesc, nullptr,
                m_offscreenRenderTarget.ReleaseAndGetAddressOf()));

        DX::ThrowIfFailed(
            device->CreateRenderTargetView(m_offscreenRenderTarget.Get(),
                &rtvDesc,
                m_offscreenRTV.ReleaseAndGetAddressOf()));

        DX::ThrowIfFailed(
            device->CreateTexture2D(&dsDesc, nullptr, m_depthBuffer.GetAddressOf()));

        DX::ThrowIfFailed(
            device->CreateDepthStencilView(m_depthBuffer.Get(),
                &dsvDesc,
                m_depthStencilSRV.ReleaseAndGetAddressOf()));

        CD3D11_RASTERIZER_DESC rastDesc(D3D11_FILL_SOLID, D3D11_CULL_BACK, FALSE,
            D3D11_DEFAULT_DEPTH_BIAS, D3D11_DEFAULT_DEPTH_BIAS_CLAMP,
            D3D11_DEFAULT_SLOPE_SCALED_DEPTH_BIAS, TRUE, FALSE, TRUE, FALSE);

        DX::ThrowIfFailed(device->CreateRasterizerState(&rastDesc,
            m_multisampledRSState.ReleaseAndGetAddressOf()));

        CD3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;

        if (multisamplingEnabled)
        {
            srvDesc = CD3D11_SHADER_RESOURCE_VIEW_DESC(m_singleSampledRT.Get(),
                D3D11_SRV_DIMENSION_TEXTURE2D,
                m_format);
            DX::ThrowIfFailed(
                device->CreateShaderResourceView(m_singleSampledRT.Get(),
                    &srvDesc,
                    m_srv.ReleaseAndGetAddressOf()));
        }
        else
        {
            srvDesc = CD3D11_SHADER_RESOURCE_VIEW_DESC(m_offscreenRenderTarget.Get(),
                D3D11_SRV_DIMENSION_TEXTURE2D,
                m_format);
            DX::ThrowIfFailed(
                device->CreateShaderResourceView(m_offscreenRenderTarget.Get(),
                    &srvDesc,
                    m_srv.ReleaseAndGetAddressOf()));
        }
    }

    ID3D11Texture2D* RenderTexture::GetTexture()
    {
        return m_offscreenRenderTarget.Get();
    }

    ID3D11RenderTargetView* RenderTexture::GetRTV()
    {
        return m_offscreenRTV.Get();
    }

    ID3D11Texture2D* RenderTexture::GetSingleSampledTexture()
    {
        if (m_multisamplingEnabled)
        {
            return m_singleSampledRT.Get();
        }
        else
        {
            return m_offscreenRenderTarget.Get();
        }
    }

    void RenderTexture::CopyToSingleSampled(ID3D11DeviceContext* context)
    {
        if (m_multisamplingEnabled)
        {
            context->ResolveSubresource(m_singleSampledRT.Get(), 0,
                m_offscreenRenderTarget.Get(), 0,
                m_format);
        }
    }

    void RenderTexture::ClearAndSetAsTarget(ID3D11DeviceContext* context)
    {
        auto renderTarget = m_offscreenRTV.Get();
        auto depthStencil = m_depthStencilSRV.Get();

        context->ClearRenderTargetView(renderTarget, DirectX::ColorsLinear::CornflowerBlue);
        context->ClearDepthStencilView(depthStencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
        context->OMSetRenderTargets(1, &renderTarget, depthStencil);
        
        if (m_multisamplingEnabled)
            context->RSSetState(m_multisampledRSState.Get());
        else
            context->RSSetState(m_states->CullClockwise());
    }

    void RenderTexture::SetAsTarget(ID3D11DeviceContext* context)
    {
        auto renderTarget = m_offscreenRTV.Get();
        auto depthStencil = m_depthStencilSRV.Get();

        context->OMSetRenderTargets(1, &renderTarget, depthStencil);

        if (m_multisamplingEnabled)
            context->RSSetState(m_multisampledRSState.Get());
        else
            context->RSSetState(m_states->CullCounterClockwise());
    }

    ID3D11ShaderResourceView* RenderTexture::GetSRV()
    {
        return m_srv.Get();
    }

    RECT RenderTexture::GetOutputSize()
    {
        return m_outputSize;
    }

    void RenderTexture::DrawTo(
        ID3D11DeviceContext* context,
        Gradient::Rendering::RenderTexture* destination,
        std::function<void __cdecl()> customState)
    {
        destination->ClearAndSetAsTarget(context);

        context->VSSetShader(nullptr, nullptr, 0);
        context->HSSetShader(nullptr, nullptr, 0);
        context->DSSetShader(nullptr, nullptr, 0);
        m_spriteBatch->Begin(DirectX::SpriteSortMode_Immediate,
            nullptr, nullptr, nullptr, nullptr, customState);
        m_spriteBatch->Draw(this->GetSRV(),
            destination->GetOutputSize());
        m_spriteBatch->End();
    }
}