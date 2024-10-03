#include "pch.h"

#include "Core/TextureManager.h"
#include <directxtk/WICTextureLoader.h>

namespace Gradient
{
    std::unique_ptr<TextureManager> TextureManager::s_textureManager;

    TextureManager::TextureManager()
    {
    }

    void TextureManager::Initialize()
    {
        auto tm = new TextureManager();
        s_textureManager = std::unique_ptr<TextureManager>(tm);
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

        DirectX::CreateWICTextureFromFile(device, 
            path.c_str(), 
            nullptr, 
            srv.ReleaseAndGetAddressOf());

        m_textureMap.insert({ key, srv });
    }

    ID3D11ShaderResourceView* TextureManager::GetTexture(std::string key)
    {
        if (auto entry = m_textureMap.find(key); entry != m_textureMap.end())
        {
            return entry->second.Get();
        }
        return nullptr;
    }
}