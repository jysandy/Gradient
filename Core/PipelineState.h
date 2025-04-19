#pragma once

#include "pch.h"

namespace Gradient
{
    class PipelineState
    {
    public:
        PipelineState(D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc);
        PipelineState(D3DX12_MESH_SHADER_PIPELINE_STATE_DESC psoDesc);

        static D3D12_GRAPHICS_PIPELINE_STATE_DESC GetDefaultDesc();
        static D3D12_GRAPHICS_PIPELINE_STATE_DESC GetDepthWriteDisableDesc();
        static D3D12_GRAPHICS_PIPELINE_STATE_DESC GetDefaultShadowDesc();

        static D3DX12_MESH_SHADER_PIPELINE_STATE_DESC GetDefaultMeshDesc();
        static D3DX12_MESH_SHADER_PIPELINE_STATE_DESC GetDepthWriteDisableMeshDesc();
        static D3DX12_MESH_SHADER_PIPELINE_STATE_DESC GetDefaultShadowMeshDesc();

        void Build(ID3D12Device2* device);
        D3D12_GRAPHICS_PIPELINE_STATE_DESC GetDesc();
        D3DX12_MESH_SHADER_PIPELINE_STATE_DESC GetMeshDesc();

        void Set(ID3D12GraphicsCommandList* cl,
            bool multisampled);

    private:
        D3D12_GRAPHICS_PIPELINE_STATE_DESC m_desc;
        D3DX12_MESH_SHADER_PIPELINE_STATE_DESC m_meshDesc;
        bool m_useMeshShader = false;

        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_singleSampledPSO;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_multisampledPSO;

        bool m_isBuilt = false;
    };
}