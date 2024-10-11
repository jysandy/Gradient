#include "pch.h"

#include "Core/TextureManager.h"
#include <directxtk/WICTextureLoader.h>

namespace Gradient
{
    std::unique_ptr<TextureManager> TextureManager::s_textureManager;

    TextureManager::TextureManager()
    {
    }

    void TextureManager::Initialize(ID3D11Device* device)
    {
        auto tm = new TextureManager();
        s_textureManager = std::unique_ptr<TextureManager>(tm);
        s_textureManager->LoadWICTexture(device, "default", L"DefaultTexture.png");
    }

    void TextureManager::Shutdown()
    {
        s_textureManager.reset();
    }

    TextureManager* TextureManager::Get()
    {
        return s_textureManager.get();
    }

    void TextureManager::LoadWICTexture(ID3D11Device* device,
        std::string key,
        std::wstring path)
    {
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;

        // Assumes the texture is not in linear space.
        DX::ThrowIfFailed(
            DirectX::CreateWICTextureFromFileEx(device,
                path.c_str(),
                0,
                D3D11_USAGE_DEFAULT,
                D3D11_BIND_SHADER_RESOURCE,
                0,
                0,
                DirectX::WIC_LOADER_FORCE_SRGB,
                nullptr,
                srv.ReleaseAndGetAddressOf()));

        m_textureMap.insert({ key, srv });
    }

    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> TextureManager::GetTexture(std::string key)
    {
        if (auto entry = m_textureMap.find(key); entry != m_textureMap.end())
        {
            return entry->second;
        }
        return nullptr;
    }
}