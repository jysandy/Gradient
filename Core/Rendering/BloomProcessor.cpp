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
            width / 4,
            height / 4,
            format,
            false
        );

        m_downsampled2 = std::make_unique<Gradient::Rendering::RenderTexture>(
            device,
            context,
            m_states,
            width / 6,
            height / 6,
            format,
            false
        );

        m_downsampled3 = std::make_unique<Gradient::Rendering::RenderTexture>(
            device,
            context,
            m_states,
            width / 4,
            height / 4,
            format,
            false
        );

        m_screensizeRenderTexture = std::make_unique<Gradient::Rendering::RenderTexture>(
            device,
            context,
            m_states,
            width,
            height,
            format,
            false
        );

        m_brightnessFilterPS = LoadPixelShader(device, L"brightness_filter.cso");
        m_additiveBlendPS = LoadPixelShader(device, L"additive_blend.cso");
        m_blurPS = LoadPixelShader(device, L"blur.cso");
        m_gaussianHorizontalPS = LoadPixelShader(device, L"gaussian_horizontal.cso");
        m_gaussianVerticalPS = LoadPixelShader(device, L"gaussian_vertical.cso");

        m_brightnessFilterCB.Create(device);
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

    void BloomProcessor::SetExposure(float bt)
    {
        m_exposure = bt;
    }

    void BloomProcessor::SetIntensity(float intensity)
    {
        m_intensity = intensity;
    }

    float BloomProcessor::GetExposure()
    {
        return m_exposure;
    }

    float BloomProcessor::GetIntensity()
    {
        return m_intensity;
    }

    RenderTexture* BloomProcessor::Process(ID3D11DeviceContext* context,
        RenderTexture* input)
    {
        input->DrawTo(context, m_downsampled1.get());

        m_downsampled1->DrawTo(context, m_downsampled2.get());
        m_downsampled2->DrawTo(context, m_downsampled1.get());

        m_downsampled1->DrawTo(context, m_downsampled3.get(),
            [=]
            {
                context->PSSetShader(m_gaussianHorizontalPS.Get(), nullptr, 0);
            });

        m_downsampled3->DrawTo(context, m_downsampled1.get(),
            [=]
            {
                context->PSSetShader(m_gaussianVerticalPS.Get(), nullptr, 0);
            });

        m_downsampled1->DrawTo(context, m_downsampled3.get(),
            [=]
            {
                m_brightnessFilterCB.SetData(context, { m_exposure, m_intensity });
                context->PSSetShader(m_brightnessFilterPS.Get(), nullptr, 0);
                auto cb = m_brightnessFilterCB.GetBuffer();
                context->PSSetConstantBuffers(0, 1, &cb);
            });

        m_downsampled3->DrawTo(context, m_downsampled1.get(),
            [=]
            {
                context->PSSetShader(m_gaussianHorizontalPS.Get(), nullptr, 0);
            });

        m_downsampled1->DrawTo(context, m_downsampled3.get(),
            [=]
            {
                context->PSSetShader(m_gaussianVerticalPS.Get(), nullptr, 0);
            });

        input->DrawTo(context,
            m_screensizeRenderTexture.get(),
            [=]
            {
                auto srv = m_downsampled3->GetSRV();
                context->PSSetShader(m_additiveBlendPS.Get(), nullptr, 0);
                context->PSSetShaderResources(1, 1, &srv);
            });

        return m_screensizeRenderTexture.get();
    }
}