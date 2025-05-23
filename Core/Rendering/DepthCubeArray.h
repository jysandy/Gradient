#pragma once

#include "pch.h"
#include <directxtk12/SimpleMath.h>
#include <functional>

#include "Core/GraphicsMemoryManager.h"
#include "Core/BarrierResource.h"

namespace Gradient::Rendering
{
    // TODO: Replace this nonsense with bindless rendering
    class DepthCubeArray
    {
    public:
        using DrawFn = std::function<void(DirectX::SimpleMath::Matrix, DirectX::SimpleMath::Matrix, DirectX::SimpleMath::Vector3)>;

        DepthCubeArray(ID3D12Device* device,
            int width,
            int numCubes);

        void Render(ID3D12GraphicsCommandList* cl,
            int cubeMapIndex,
            DirectX::SimpleMath::Vector3 origin,
            float nearPlane,
            float farPlane,
            DrawFn fn);

        GraphicsMemoryManager::DescriptorView GetSRV() const;
        void TransitionToShaderResource(ID3D12GraphicsCommandList* cl);

    private:
        BarrierResource m_cubemapArray;
        GraphicsMemoryManager::DescriptorView m_srv;
        std::vector<GraphicsMemoryManager::DescriptorView> m_dsvs;

        D3D12_VIEWPORT m_viewport;
    };
}