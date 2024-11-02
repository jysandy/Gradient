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
            m_states,
            width / 2,
            height / 2,
            DXGI_FORMAT_R32G32B32A32_FLOAT,
            false
        );

        m_downsampled2 = std::make_unique<Gradient::Rendering::RenderTexture>(
            device,
            m_states,
            width / 4,
            height / 4,
            DXGI_FORMAT_R32G32B32A32_FLOAT,
            false
        );

        m_screensizeRenderTexture = std::make_unique<Gradient::Rendering::RenderTexture>(
            device,
            m_states,
            width,
            height,
            DXGI_FORMAT_R32G32B32A32_FLOAT,
            false
        );

        m_screensize2RenderTexture = std::make_unique<Gradient::Rendering::RenderTexture>(
            device,
            m_states,
            width,
            height,
            DXGI_FORMAT_R32G32B32A32_FLOAT,
            false
        );

        m_spriteBatch = std::make_unique<DirectX::SpriteBatch>(context);


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

    void BloomProcessor::DrawRenderTexture(
        ID3D11DeviceContext* context,
        Gradient::Rendering::RenderTexture* source,
        Gradient::Rendering::RenderTexture* destination,
        std::function<void __cdecl()> customState)
    {
        destination->ClearAndSetTargets(context);

        m_spriteBatch->Begin(DirectX::SpriteSortMode_Immediate,
            nullptr, nullptr, nullptr, nullptr, customState);
        m_spriteBatch->Draw(source->GetSRV(),
            destination->GetOutputSize());
        m_spriteBatch->End();
    }

    RenderTexture* BloomProcessor::Process(ID3D11DeviceContext* context,
        RenderTexture* input)
    {
        DrawRenderTexture(context,
            input,
            m_downsampled1.get());

        DrawRenderTexture(context, 
            m_downsampled1.get(),
            m_downsampled2.get());

        DrawRenderTexture(context, 
            m_downsampled2.get(),
            m_downsampled1.get());

        DrawRenderTexture(context, 
            m_downsampled1.get(),
            m_screensizeRenderTexture.get());

        DrawRenderTexture(context, 
            m_screensizeRenderTexture.get(),
            m_screensize2RenderTexture.get(),
            [=]
            {
                context->PSSetShader(m_brightnessFilterPS.Get(), nullptr, 0);
            });

        DrawRenderTexture(context, 
            input,
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