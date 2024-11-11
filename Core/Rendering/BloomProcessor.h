#pragma once

#include "pch.h"
#include "Core/Rendering/RenderTexture.h"
#include <directxtk/CommonStates.h>
#include <directxtk/SpriteBatch.h>
#include <directxtk/BufferHelpers.h>

namespace Gradient::Rendering
{
    class BloomProcessor
    {
    public:
        BloomProcessor(ID3D11Device* device,
            ID3D11DeviceContext* context,
            std::shared_ptr<DirectX::CommonStates> states,
            int width,
            int height,
            DXGI_FORMAT format
        );

        RenderTexture* Process(ID3D11DeviceContext* context,
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

        Microsoft::WRL::ComPtr<ID3D11PixelShader> m_brightnessFilterPS;
        Microsoft::WRL::ComPtr<ID3D11PixelShader> m_additiveBlendPS;
        Microsoft::WRL::ComPtr<ID3D11PixelShader> m_blurPS;
        Microsoft::WRL::ComPtr<ID3D11PixelShader> m_gaussianHorizontalPS;
        Microsoft::WRL::ComPtr<ID3D11PixelShader> m_gaussianVerticalPS;

        Microsoft::WRL::ComPtr<ID3D11PixelShader> LoadPixelShader(
            ID3D11Device* device,
            const std::wstring& path);

        DirectX::ConstantBuffer<BloomParams> m_brightnessFilterCB;
        float m_exposure;
        float m_intensity;
    };
}