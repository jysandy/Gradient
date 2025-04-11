#include "pch.h"

#include "Core/Pipelines/IRenderPipeline.h"

namespace Gradient::Pipelines
{
    void IRenderPipeline::SetMaterial(const Rendering::PBRMaterial& material)
    {
        // Ignored by default;
    }
}