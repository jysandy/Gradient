#include "pch.h"

#include "Core/Pipelines/IRenderPipeline.h"

namespace Gradient::Pipelines
{
    void IRenderPipeline::SetAlbedo(GraphicsMemoryManager::DescriptorView index)
    {
        // Ignored
    }

    void IRenderPipeline::SetNormalMap(GraphicsMemoryManager::DescriptorView index)
    {
        // Ignored
    }

    void IRenderPipeline::SetAOMap(GraphicsMemoryManager::DescriptorView index)
    {
        // Ignored
    }

    void IRenderPipeline::SetMetalnessMap(GraphicsMemoryManager::DescriptorView index)
    {
        // Ignored
    }

    void IRenderPipeline::SetRoughnessMap(GraphicsMemoryManager::DescriptorView index)
    {
        // Ignored
    }

    void IRenderPipeline::SetEmissiveRadiance(DirectX::SimpleMath::Vector3 radiance)
    {
        // Ignored
    }
}