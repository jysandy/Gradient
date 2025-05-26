#pragma once

#include "pch.h"
#include "Core/Camera.h"
#include "Core/BarrierResource.h"
#include "Core/GraphicsMemoryManager.h"
#include "Core/RootSignature.h"
#include "Core/Shaders/XeGTAO.h"

namespace Gradient::Rendering
{
    class GTAOProcessor
    {
    public:
        GTAOProcessor(ID3D12Device* device,
            RECT windowSize);

        void Process(
            ID3D12GraphicsCommandList* cl,
            BarrierResource* depthBuffer,
            const Camera* viewingCamera,
            RECT windowSize);

        GraphicsMemoryManager::DescriptorView GetSRV(ID3D12GraphicsCommandList* cl);

    private:

        XeGTAO::GTAOSettings m_gtaoSettings;

        BarrierResource m_inputDepthBuffer;
        GraphicsMemoryManager::DescriptorView m_inputDepthBufferSRV;

        BarrierResource m_normals;
        GraphicsMemoryManager::DescriptorView m_normalsUAV;
        GraphicsMemoryManager::DescriptorView m_normalsSRV;

        BarrierResource m_workingDepths;
        GraphicsMemoryManager::DescriptorView m_workingDepthsSRV;
        GraphicsMemoryManager::DescriptorView m_workingDepthsUAVs[XE_GTAO_DEPTH_MIP_LEVELS];

        BarrierResource m_workingAOTerm;
        GraphicsMemoryManager::DescriptorView m_workingAOTermSRV;
        GraphicsMemoryManager::DescriptorView m_workingAOTermUAV;

        BarrierResource m_workingAOTermPong;
        GraphicsMemoryManager::DescriptorView m_workingAOTermPongSRV;
        GraphicsMemoryManager::DescriptorView m_workingAOTermPongUAV;

        bool m_readFromPong = false;

        BarrierResource m_edges;
        GraphicsMemoryManager::DescriptorView m_edgesUAV;
        GraphicsMemoryManager::DescriptorView m_edgesSRV;

        RootSignature m_generateNormalsRS;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_generateNormalsPSO;

        RootSignature m_prefilterDepthsRS;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_prefilterDepthsPSO;

        RootSignature m_mainPassRS;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_mainPassPSO;

        RootSignature m_denoisePassRS;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_denoisePassPSO;
    };
}