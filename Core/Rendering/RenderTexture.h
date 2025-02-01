#pragma once
#include "pch.h"
#include "Core/BarrierResource.h"
#include "Core/GraphicsMemoryManager.h"
#include "Core/Rendering/TextureDrawer.h"
#include <directxtk12/CommonStates.h>
#include <directxtk12/SpriteBatch.h>
#include <memory>

namespace Gradient::Rendering
{
    class RenderTexture
    {
    public:
        RenderTexture(
            ID3D12Device* device,
            UINT width,
            UINT height,
            DXGI_FORMAT format,
            bool multisamplingEnabled);

        ID3D12Resource* GetTexture();
        BarrierResource* GetBarrierResource();
        GraphicsMemoryManager::DescriptorIndex GetRTV();
        ID3D12Resource* GetSingleSampledTexture();
        BarrierResource* GetSingleSampledBarrierResource();
        GraphicsMemoryManager::DescriptorIndex GetSRV();
        RECT GetOutputSize();
        void CopyToSingleSampled(ID3D12GraphicsCommandList* cl);
        void ClearAndSetAsTarget(ID3D12GraphicsCommandList* cl);
        void SetAsTarget(ID3D12GraphicsCommandList* cl);
        void DrawTo(ID3D12GraphicsCommandList* cl,
            RenderTexture* destination,
            TextureDrawer* texDrawer);

    private:
        bool m_multisamplingEnabled;
        DXGI_FORMAT m_format;
        RECT m_size;

        BarrierResource m_offscreenRT;
        BarrierResource m_singleSampledRT;
        GraphicsMemoryManager::DescriptorIndex m_rtv;
        GraphicsMemoryManager::DescriptorIndex m_srv;

        BarrierResource m_depthBuffer;
        GraphicsMemoryManager::DescriptorIndex m_dsv;
    };
}