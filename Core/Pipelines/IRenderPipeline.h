#pragma once

#include "pch.h"

#include "Core/Rendering/IDrawable.h"
#include "Core/GraphicsMemoryManager.h"
#include "Core/Rendering/PBRMaterial.h"
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
        enum class PassType
        {
            ShadowPass,
            ZPrePass,
            ForwardPass
        };

        virtual ~IRenderPipeline() noexcept = default;

        IRenderPipeline(const IRenderPipeline&) = delete;
        IRenderPipeline& operator=(const IRenderPipeline&) = delete;

        virtual void Apply(ID3D12GraphicsCommandList* cl, bool multisampled = true, PassType passType = PassType::ForwardPass) = 0;

        virtual void SetMaterial(const Rendering::PBRMaterial& material);

    protected:
        IRenderPipeline() = default;
        IRenderPipeline(IRenderPipeline&&) = default;
        IRenderPipeline& operator=(IRenderPipeline&&) = default;
    };
}