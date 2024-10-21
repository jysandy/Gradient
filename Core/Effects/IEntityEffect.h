#pragma once

#include "pch.h"

#include <directxtk/Effects.h>

namespace Gradient::Effects
{
    // Interface for effects that can be used by the EntityManager for rendering entities.
    class IEntityEffect : public DirectX::IEffect,
        public DirectX::IEffectMatrices
    {
    public:
        virtual ~IEntityEffect() = default;

        IEntityEffect(const IEntityEffect&) = delete;
        IEntityEffect& operator=(const IEntityEffect&) = delete;

        virtual void SetTexture(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv) = 0;
        virtual void SetNormalMap(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv) = 0;
        virtual void SetAOMap(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv) = 0;
        virtual ID3D11InputLayout* GetInputLayout() const = 0;

    protected:
        IEntityEffect() = default;
        IEntityEffect(IEntityEffect&&) = default;
        IEntityEffect& operator=(IEntityEffect&&) = default;
    };
}