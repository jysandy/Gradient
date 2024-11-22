#pragma once

#include "pch.h"

#include "Core/Rendering/IDrawable.h"
#include <directxtk/Effects.h>

namespace Gradient::Pipelines
{
    
    // Abstract class representing a rendering pipeline, 
    // which includes shaders, fixed-function states and 
    // other configuration.
    class IRenderPipeline : public DirectX::IEffectMatrices
    {
    public:
        virtual ~IRenderPipeline() = default;

        IRenderPipeline(const IRenderPipeline&) = delete;
        IRenderPipeline& operator=(const IRenderPipeline&) = delete;

        virtual void Apply(ID3D11DeviceContext* context) = 0;

        virtual void SetAlbedo(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv);
        virtual void SetNormalMap(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv);
        virtual void SetAOMap(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv);
        virtual void SetMetalnessMap(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv);
        virtual void SetRoughnessMap(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv);
        virtual ID3D11InputLayout* GetInputLayout() const = 0;

    protected:
        IRenderPipeline() = default;
        IRenderPipeline(IRenderPipeline&&) = default;
        IRenderPipeline& operator=(IRenderPipeline&&) = default;
    };
}