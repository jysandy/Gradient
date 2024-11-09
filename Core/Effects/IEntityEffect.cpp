#include "pch.h"

#include "Core/Effects/IEntityEffect.h"

namespace Gradient::Effects
{
    void IEntityEffect::SetAlbedo(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv)
    {
        // Ignored
    }

    void IEntityEffect::SetNormalMap(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv)
    {
        // Ignored
    }

    void IEntityEffect::SetAOMap(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv)
    {
        // Ignored
    }

    void IEntityEffect::SetMetalnessMap(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv)
    {
        // Ignored
    }

    void IEntityEffect::SetRoughnessMap(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv)
    {
        // Ignored
    }
}