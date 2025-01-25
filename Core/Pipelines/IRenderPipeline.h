#pragma once

#include "pch.h"

#include "Core/Rendering/IDrawable.h"
#include "Core/GraphicsMemoryManager.h"
#include <optional>
#include <directxtk12/Effects.h>
#include <directxtk12/SimpleMath.h>

namespace Gradient::Pipelines
{
    
    // Abstract class representing a rendering pipeline, 
    // which includes shaders, fixed-function states and 
    // other configuration.
    class IRenderPipeline : public DirectX::IEffectMatrices
    {
    public:
        virtual ~IRenderPipeline() noexcept = default;

        IRenderPipeline(const IRenderPipeline&) = delete;
        IRenderPipeline& operator=(const IRenderPipeline&) = delete;

        virtual void Apply(ID3D12GraphicsCommandList* cl) = 0;

        virtual void SetAlbedo(std::optional<GraphicsMemoryManager::DescriptorIndex> index);
        virtual void SetNormalMap(std::optional<GraphicsMemoryManager::DescriptorIndex> index);
        virtual void SetAOMap(std::optional<GraphicsMemoryManager::DescriptorIndex> index);
        virtual void SetMetalnessMap(std::optional<GraphicsMemoryManager::DescriptorIndex> index);
        virtual void SetRoughnessMap(std::optional<GraphicsMemoryManager::DescriptorIndex> index);
        virtual void SetEmissiveRadiance(DirectX::SimpleMath::Vector3 radiance);

    protected:
        IRenderPipeline() = default;
        IRenderPipeline(IRenderPipeline&&) = default;
        IRenderPipeline& operator=(IRenderPipeline&&) = default;
    };
}