#pragma once
#include "pch.h"
#include <directxtk/CommonStates.h>
#include <directxtk/SpriteBatch.h>
#include <memory>

namespace Gradient::Rendering
{
    class RenderTexture
    {
    public:
        RenderTexture(
            ID3D11Device* device,
            ID3D11DeviceContext* context,
            std::shared_ptr<DirectX::CommonStates> commonStates,
            UINT width,
            UINT height,
            DXGI_FORMAT format,
            bool multisamplingEnabled);

        ID3D11Texture2D* GetTexture();
        ID3D11RenderTargetView* GetRTV();
        ID3D11Texture2D* GetSingleSampledTexture();
        ID3D11ShaderResourceView* GetSRV();
        RECT GetOutputSize();
        void CopyToSingleSampled(ID3D11DeviceContext* context);
        void ClearAndSetAsTarget(ID3D11DeviceContext* context);
        void SetAsTarget(ID3D11DeviceContext* context);
        void DrawTo(ID3D11DeviceContext* context,
            RenderTexture* destination,
            std::function<void __cdecl()> customState = nullptr);

    private:
        bool m_multisamplingEnabled;
        DXGI_FORMAT m_format;
        std::shared_ptr<DirectX::CommonStates> m_states;
        RECT m_outputSize;
        std::unique_ptr<DirectX::SpriteBatch> m_spriteBatch;

        Microsoft::WRL::ComPtr<ID3D11Texture2D> m_offscreenRenderTarget;
        Microsoft::WRL::ComPtr<ID3D11Texture2D> m_singleSampledRT;
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_offscreenRTV;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_srv;

        Microsoft::WRL::ComPtr<ID3D11Texture2D> m_depthBuffer;
        Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_depthStencilSRV;
        Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_multisampledRSState;
    };
}