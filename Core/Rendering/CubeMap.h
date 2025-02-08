#pragma once

#include "pch.h"
#include "Core/BarrierResource.h"
#include "Core/GraphicsMemoryManager.h"
#include <directxtk12/SimpleMath.h>
#include <functional>

namespace Gradient::Rendering
{
    class CubeMap
    {
    public:
        using DrawFn = std::function<void(DirectX::SimpleMath::Matrix, 
            DirectX::SimpleMath::Matrix)>;

        CubeMap(ID3D12Device* device,
            int width,
            DXGI_FORMAT format);

        void Render(ID3D12GraphicsCommandList* cl, DrawFn fn);
        GraphicsMemoryManager::DescriptorView GetSRV() const;
        void TransitionToShaderResource(ID3D12GraphicsCommandList* cl);

    private:
        BarrierResource m_texture;
        BarrierResource m_depthTex;
        GraphicsMemoryManager::DescriptorView m_srv;
        GraphicsMemoryManager::DescriptorView m_rtv[6];
        GraphicsMemoryManager::DescriptorView m_dsv;

        D3D12_VIEWPORT m_viewport;
    };
}