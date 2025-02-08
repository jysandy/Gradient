#pragma once

#include "pch.h"

#include "Core/GraphicsMemoryManager.h"
#include "Core/RootSignature.h"

#include <directxtk12/SpriteBatch.h>
#include <string>

// Draws a texture, optionally with a custom pixel shader.
namespace Gradient::Rendering
{
    class TextureDrawer
    {
    public:
        TextureDrawer(
            ID3D12Device* device,
            ID3D12CommandQueue* cq,
            DXGI_FORMAT format,
            std::wstring shaderPath);

        TextureDrawer(
            ID3D12Device* device,
            ID3D12CommandQueue* cq,
            DXGI_FORMAT format);

        void Draw(ID3D12GraphicsCommandList* cl,
            GraphicsMemoryManager::DescriptorView texSRV,
            D3D12_VIEWPORT viewport,
            RECT inputSize,
            RECT outputSize);

        static void CreateRootSignature(ID3D12Device* device);
        static void Shutdown();

        template <typename T>
        static void SetCBV(ID3D12GraphicsCommandList* cl,
            const T& data);

        static void SetSRV(ID3D12GraphicsCommandList* cl,
            GraphicsMemoryManager::DescriptorView index);

    private:
        std::unique_ptr<DirectX::SpriteBatch> m_spriteBatch;
        
        static RootSignature s_rootSignature;
    };

    template <typename T>
    void TextureDrawer::SetCBV(ID3D12GraphicsCommandList* cl,
        const T& data)
    {
        s_rootSignature.SetCBV(cl, 0, 1, data);
    }
}