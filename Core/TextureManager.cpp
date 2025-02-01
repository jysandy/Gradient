#include "pch.h"

#include "Core/TextureManager.h"
#include <directxtk12/WICTextureLoader.h>
#include <directxtk12/DDSTextureLoader.h>
#include <directxtk12/ResourceUploadBatch.h>

namespace Gradient
{
    std::unique_ptr<TextureManager> TextureManager::s_textureManager;

    TextureManager::TextureManager(ID3D12Device* device)
    {
        DX::ThrowIfFailed(
            device->CreateCommandAllocator(
                D3D12_COMMAND_LIST_TYPE_DIRECT,
                IID_PPV_ARGS(m_commandAllocator.ReleaseAndGetAddressOf())));

        DX::ThrowIfFailed(
            device->CreateCommandList(
                0,
                D3D12_COMMAND_LIST_TYPE_DIRECT,
                m_commandAllocator.Get(),
                nullptr,
                IID_PPV_ARGS(m_commandList.ReleaseAndGetAddressOf())));
        m_commandList->Close();

        m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

        DX::ThrowIfFailed(
            device->CreateFence(0,
                D3D12_FENCE_FLAG_NONE,
                IID_PPV_ARGS(m_fence.ReleaseAndGetAddressOf())));
    }

    void TextureManager::WaitForGPU(ID3D12CommandQueue* cq)
    {
        cq->Signal(m_fence.Get(), m_fenceValue);
        if (m_fence->GetCompletedValue() < m_fenceValue)
        {
            m_fence->SetEventOnCompletion(m_fenceValue, m_fenceEvent);
            WaitForSingleObject(m_fenceEvent, INFINITE);
        }
    }

    void TextureManager::ResetCommandList(ID3D12CommandQueue* cq)
    {
        WaitForGPU(cq);
        m_commandAllocator->Reset();
        m_commandList->Reset(m_commandAllocator.Get(), nullptr);
    }

    void TextureManager::SubmitCommandList(ID3D12CommandQueue* cq)
    {
        m_commandList->Close();
        ID3D12CommandList* commandLists[] = { m_commandList.Get() };
        cq->ExecuteCommandLists(1, commandLists);
        m_fenceValue++;
    }

    void TextureManager::Initialize(ID3D12Device* device,
        ID3D12CommandQueue* cq)
    {
        auto tm = new TextureManager(device);
        s_textureManager = std::unique_ptr<TextureManager>(tm);
        s_textureManager->LoadWIC(device,
            cq,
            "default",
            L"Assets\\DefaultTexture.png",
            true);
        s_textureManager->LoadWIC(device,
            cq,
            "defaultNormal",
            L"Assets\\DefaultNormalMap.png");
        s_textureManager->LoadWIC(device,
            cq,
            "defaultMetalness",
            L"Assets\\DefaultMetalness.bmp");
    }

    void TextureManager::Shutdown()
    {
        s_textureManager.reset();
    }

    TextureManager* TextureManager::Get()
    {
        return s_textureManager.get();
    }

    void TextureManager::LoadWIC(ID3D12Device* device,
        ID3D12CommandQueue* cq,
        std::string key,
        std::wstring path,
        bool sRGB)
    {
        Microsoft::WRL::ComPtr<ID3D12Resource> resource;
        DirectX::ResourceUploadBatch uploadBatch(device);

        uploadBatch.Begin();

        DX::ThrowIfFailed(
            DirectX::CreateWICTextureFromFileEx(device,
                uploadBatch,
                path.c_str(),
                0,
                D3D12_RESOURCE_FLAG_NONE,
                DirectX::WIC_LOADER_MIP_AUTOGEN | (sRGB ? DirectX::WIC_LOADER_FORCE_SRGB : DirectX::WIC_LOADER_IGNORE_SRGB),
                resource.ReleaseAndGetAddressOf()));

        auto uploadFinished = uploadBatch.End(cq);
        uploadFinished.wait();

        ResetCommandList(cq);

        DirectX::TransitionResource(m_commandList.Get(),
            resource.Get(),
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);

        SubmitCommandList(cq);

        auto gmm = GraphicsMemoryManager::Get();
        auto srvIndex = gmm->CreateSRV(device, resource.Get());

        m_textureMap.insert({ key, {resource, srvIndex} });
    }

    void TextureManager::LoadDDS(ID3D12Device* device,
        ID3D12CommandQueue* cq,
        std::string key,
        std::wstring path)
    {
        Microsoft::WRL::ComPtr<ID3D12Resource> resource;
        DirectX::ResourceUploadBatch uploadBatch(device);

        uploadBatch.Begin();

        DX::ThrowIfFailed(
            DirectX::CreateDDSTextureFromFile(device,
                uploadBatch,
                path.c_str(),
                resource.ReleaseAndGetAddressOf()));

        auto uploadFinished = uploadBatch.End(cq);
        uploadFinished.wait();

        ResetCommandList(cq);

        DirectX::TransitionResource(m_commandList.Get(),
            resource.Get(),
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);

        SubmitCommandList(cq);

        auto gmm = GraphicsMemoryManager::Get();
        auto srvIndex = gmm->CreateSRV(device, resource.Get());

        m_textureMap.insert({ key, {resource, srvIndex} });
    }

    std::optional<GraphicsMemoryManager::DescriptorIndex>
        TextureManager::GetTexture(std::string key)
    {
        if (auto entry = m_textureMap.find(key); entry != m_textureMap.end())
        {
            return entry->second.SrvIndex;
        }
        return std::nullopt;
    }
}