#include "pch.h"

#include <memory>
#include "Core/Rendering/BloomProcessor.h"
#include "Core/ReadData.h"

namespace Gradient::Rendering
{
    BloomProcessor::BloomProcessor(ID3D11Device* device,
        ID3D11DeviceContext* context,
        std::shared_ptr<DirectX::CommonStates> states,
        int width,
        int height,
        DXGI_FORMAT format) : m_states(states)
    {
        m_downsampled1 = std::make_unique<Gradient::Rendering::RenderTexture>(
            device,
            context,
            m_states,
            width / 2,
            height / 2,
            DXGI_FORMAT_R32G32B32A32_FLOAT,
            false
        );

        m_downsampled2 = std::make_unique<Gradient::Rendering::RenderTexture>(
            device,
            context,
            m_states,
            width / 4,
            height / 4,
            DXGI_FORMAT_R32G32B32A32_FLOAT,
            false
        );

        m_screensizeRenderTexture = std::make_unique<Gradient::Rendering::RenderTexture>(
            device,
            context,
            m_states,
            width,
            height,
            DXGI_FORMAT_R32G32B32A32_FLOAT,
            false
        );

        m_screensize2RenderTexture = std::make_unique<Gradient::Rendering::RenderTexture>(
            device,
            context,
            m_states,
            width,
            height,
            DXGI_FORMAT_R32G32B32A32_FLOAT,
            false
        );

        m_brightnessFilterPS = LoadPixelShader(device, L"brightness_filter.cso");
        m_additiveBlendPS = LoadPixelShader(device, L"additive_blend.cso");
    }

    Microsoft::WRL::ComPtr<ID3D11PixelShader> BloomProcessor::LoadPixelShader(
        ID3D11Device* device,
        const std::wstring& path)
    {
        Microsoft::WRL::ComPtr<ID3D11PixelShader> ps;

        auto psData = DX::ReadData(path.c_str());
        DX::ThrowIfFailed(
            device->CreatePixelShader(psData.data(),
                psData.size(),
                nullptr,
                ps.ReleaseAndGetAddressOf()));

        return ps;
    }

    RenderTexture* BloomProcessor::Process(ID3D11DeviceContext* context,
        RenderTexture* input)
    {
        input->DrawTo(context, m_downsampled1.get());
        m_downsampled1->DrawTo(context, m_downsampled2.get());
        m_downsampled2->DrawTo(context, m_downsampled1.get());
        m_downsampled1->DrawTo(context, m_screensizeRenderTexture.get());
        m_screensizeRenderTexture->DrawTo(context,
            m_screensize2RenderTexture.get(),
            [=]
            {
                context->PSSetShader(m_brightnessFilterPS.Get(), nullptr, 0);
            });
        
        m_screensize2RenderTexture->DrawTo(context, m_downsampled1.get());
        m_downsampled1->DrawTo(context, m_downsampled2.get());
        m_downsampled2->DrawTo(context, m_downsampled1.get());
        m_downsampled1->DrawTo(context, m_screensize2RenderTexture.get());

        input->DrawTo(context,
            m_screensizeRenderTexture.get(),
            [=]
            {
                auto srv = m_screensize2RenderTexture->GetSRV();
                context->PSSetShader(m_additiveBlendPS.Get(), nullptr, 0);
                context->PSSetShaderResources(1, 1, &srv);
            });

        return m_screensizeRenderTexture.get();
    }
}