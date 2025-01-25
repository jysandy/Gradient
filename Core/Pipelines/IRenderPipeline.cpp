#include "pch.h"

#include "Core/Pipelines/IRenderPipeline.h"

namespace Gradient::Pipelines
{
    void IRenderPipeline::SetAlbedo(std::optional<GraphicsMemoryManager::DescriptorIndex> index)
    {
        // Ignored
    }

    void IRenderPipeline::SetNormalMap(std::optional<GraphicsMemoryManager::DescriptorIndex> index)
    {
        // Ignored
    }

    void IRenderPipeline::SetAOMap(std::optional<GraphicsMemoryManager::DescriptorIndex> index)
    {
        // Ignored
    }

    void IRenderPipeline::SetMetalnessMap(std::optional<GraphicsMemoryManager::DescriptorIndex> index)
    {
        // Ignored
    }

    void IRenderPipeline::SetRoughnessMap(std::optional<GraphicsMemoryManager::DescriptorIndex> index)
    {
        // Ignored
    }

    void IRenderPipeline::SetEmissiveRadiance(DirectX::SimpleMath::Vector3 radiance)
    {
        // Ignored
    }
}