#pragma once

#include "pch.h"
#include "Core/Rendering/RenderTexture.h"
#include "Core/Rendering/TextureDrawer.h"
#include <directxtk12/CommonStates.h>
#include <directxtk12/SpriteBatch.h>
#include <directxtk12/BufferHelpers.h>

namespace Gradient::Rendering
{
    class BloomProcessor
    {
    public:
        BloomProcessor(ID3D12Device* device,
            ID3D12CommandQueue* cq,
            int width,
            int height,
            DXGI_FORMAT format
        );

        RenderTexture* Process(ID3D12GraphicsCommandList* cl,
            RenderTexture* input);

        float GetExposure();
        float GetIntensity();

        void SetExposure(float bt);
        void SetIntensity(float intensity);

    private:
        struct __declspec(align(16)) BloomParams
        {
            float exposure;
            float intensity;
        };

        std::shared_ptr<DirectX::CommonStates> m_states;
        std::unique_ptr<Gradient::Rendering::RenderTexture> m_downsampled1;
        std::unique_ptr<Gradient::Rendering::RenderTexture> m_downsampled2;
        std::unique_ptr<Gradient::Rendering::RenderTexture> m_downsampled3;
        std::unique_ptr<Gradient::Rendering::RenderTexture> m_screensizeRenderTexture;

        std::unique_ptr<TextureDrawer> m_brightnessFilter;
        std::unique_ptr<TextureDrawer> m_additiveBlend;
        std::unique_ptr<TextureDrawer> m_gaussianHorizontal;
        std::unique_ptr<TextureDrawer> m_gaussianVertical;
        std::unique_ptr<TextureDrawer> m_vanilla;

        float m_exposure;
        float m_intensity;
    };
}