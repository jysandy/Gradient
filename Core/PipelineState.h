#pragma once

#include "pch.h"

namespace Gradient
{
    class PipelineState
    {
    public:
        PipelineState(D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc);

        static D3D12_GRAPHICS_PIPELINE_STATE_DESC GetDefaultDesc();
        static D3D12_GRAPHICS_PIPELINE_STATE_DESC GetDefaultShadowDesc();

        void Build(ID3D12Device* device);
        D3D12_GRAPHICS_PIPELINE_STATE_DESC GetDesc();

        void Set(ID3D12GraphicsCommandList* cl,
            bool multisampled);

    private:
        D3D12_GRAPHICS_PIPELINE_STATE_DESC m_desc;

        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_singleSampledPSO;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_multisampledPSO;
    };
}