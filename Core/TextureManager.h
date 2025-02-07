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
            GraphicsMemoryManager::DescriptorView SrvIndex;
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
        GraphicsMemoryManager::DescriptorView
            GetTexture(std::string key);

    private:
        static std::unique_ptr<TextureManager> s_textureManager;

        TextureManager(ID3D12Device* device);
        void WaitForGPU(ID3D12CommandQueue* cq);
        void ResetCommandList(ID3D12CommandQueue* cq);
        void SubmitCommandList(ID3D12CommandQueue* cq);

        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandAllocator;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
        Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
        HANDLE m_fenceEvent;
        UINT64 m_fenceValue = 1;
        std::unordered_map<std::string, TextureMapEntry> m_textureMap;
    };
}