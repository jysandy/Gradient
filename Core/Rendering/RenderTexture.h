#pragma once
#include "pch.h"
#include <directxtk/CommonStates.h>

namespace Gradient::Rendering
{
    class RenderTexture
    {
    public:
        RenderTexture(
            ID3D11Device* device,
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
        void ClearAndSetTargets(ID3D11DeviceContext* context);
        void SetTargets(ID3D11DeviceContext* context);

    private:
        bool m_multisamplingEnabled;
        DXGI_FORMAT m_format;
        std::shared_ptr<DirectX::CommonStates> m_states;
        RECT m_outputSize;

        Microsoft::WRL::ComPtr<ID3D11Texture2D> m_offscreenRenderTarget;
        Microsoft::WRL::ComPtr<ID3D11Texture2D> m_singleSampledRT;
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_offscreenRTV;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_srv;

        Microsoft::WRL::ComPtr<ID3D11Texture2D> m_depthBuffer;
        Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_depthStencilSRV;
        Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_multisampledRSState;
    };
}