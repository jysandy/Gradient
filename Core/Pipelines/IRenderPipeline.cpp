#include "pch.h"

#include "Core/Pipelines/IRenderPipeline.h"

namespace Gradient::Pipelines
{
    void IRenderPipeline::SetAlbedo(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv)
    {
        // Ignored
    }

    void IRenderPipeline::SetNormalMap(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv)
    {
        // Ignored
    }

    void IRenderPipeline::SetAOMap(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv)
    {
        // Ignored
    }

    void IRenderPipeline::SetMetalnessMap(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv)
    {
        // Ignored
    }

    void IRenderPipeline::SetRoughnessMap(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv)
    {
        // Ignored
    }
}