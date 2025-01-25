#pragma once

#include "pch.h"
              
#include "Core/GraphicsMemoryManager.h"
#include <unordered_map>
#include <optional>

namespace Gradient
{
    class TextureManager
    {
    public:
        struct TextureMapEntry
        {
            Microsoft::WRL::ComPtr<ID3D12Resource> Resource;
            GraphicsMemoryManager::DescriptorIndex SrvIndex;
        };

        static void Initialize(ID3D12Device* device,
            ID3D12CommandQueue* cq);
        static void Shutdown();

        static TextureManager* Get();
        void LoadWIC(ID3D12Device* device,
            ID3D12CommandQueue* cq,
            std::string key,
            std::wstring path,
            bool sRGB = false);
        void LoadDDS(ID3D12Device* device, 
            ID3D12CommandQueue* cq, 
            std::string key, 
            std::wstring path);
        std::optional<GraphicsMemoryManager::DescriptorIndex>
            GetTexture(std::string key);

    private:
        static std::unique_ptr<TextureManager> s_textureManager;

        TextureManager();

        std::unordered_map<std::string, TextureMapEntry> m_textureMap;
    };
}