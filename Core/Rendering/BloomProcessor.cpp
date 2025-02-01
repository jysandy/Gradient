#include "pch.h"

#include <memory>
#include "Core/Rendering/BloomProcessor.h"
#include "Core/ReadData.h"

namespace Gradient::Rendering
{
    BloomProcessor::BloomProcessor(ID3D12Device* device,
        ID3D12CommandQueue* cq,
        int width,
        int height,
        DXGI_FORMAT format)
    {
        m_downsampled1 = std::make_unique<Gradient::Rendering::RenderTexture>(
            device,
            width / 4,
            height / 4,
            format,
            false
        );

        m_downsampled2 = std::make_unique<Gradient::Rendering::RenderTexture>(
            device,
            width / 6,
            height / 6,
            format,
            false
        );

        m_downsampled3 = std::make_unique<Gradient::Rendering::RenderTexture>(
            device,
            width / 4,
            height / 4,
            format,
            false
        );

        m_screensizeRenderTexture = std::make_unique<Gradient::Rendering::RenderTexture>(
            device,
            width,
            height,
            format,
            false
        );

        m_brightnessFilter = std::make_unique<TextureDrawer>(
            device,
            cq,
            format,
            L"BrightnessFilter_PS.cso"
        );

        m_additiveBlend = std::make_unique<TextureDrawer>(
            device,
            cq,
            format,
            L"AdditiveBlend_PS.cso"
        );

        m_gaussianHorizontal = std::make_unique<TextureDrawer>(
            device,
            cq,
            format,
            L"GaussianHorizontal_PS.cso"
        );

        m_gaussianVertical = std::make_unique<TextureDrawer>(
            device,
            cq,
            format,
            L"GaussianVertical_PS.cso"
        );

        m_vanilla = std::make_unique<TextureDrawer>(
            device,
            cq,
            format
        );
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

    RenderTexture* BloomProcessor::Process(ID3D12GraphicsCommandList* cl,
        RenderTexture* input)
    {
        input->DrawTo(cl, m_downsampled1.get(), m_vanilla.get());

        m_downsampled1->DrawTo(cl, m_downsampled2.get(), m_vanilla.get());
        m_downsampled2->DrawTo(cl, m_downsampled1.get(), m_vanilla.get());

        m_downsampled1->DrawTo(cl, m_downsampled3.get(),
            m_gaussianHorizontal.get());

        m_downsampled3->DrawTo(cl, m_downsampled1.get(),
            m_gaussianVertical.get());

        TextureDrawer::SetCBV(cl, BloomParams{ m_exposure, m_intensity });

        m_downsampled1->DrawTo(cl,
            m_downsampled3.get(),
            m_brightnessFilter.get()
        );

        m_downsampled3->DrawTo(cl, m_downsampled1.get(),
            m_gaussianHorizontal.get()
        );

        m_downsampled1->DrawTo(cl, m_downsampled3.get(),
            m_gaussianVertical.get()
        );

        m_downsampled3
            ->GetSingleSampledBarrierResource()
            ->Transition(cl, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        auto srv = m_downsampled3->GetSRV();
        TextureDrawer::SetSRV(cl, srv);

        input->DrawTo(cl,
            m_screensizeRenderTexture.get(),
            m_additiveBlend.get()
        );

        return m_screensizeRenderTexture.get();
    }
}