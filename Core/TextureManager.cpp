#include "pch.h"

#include "Core/TextureManager.h"
#include <directxtk/WICTextureLoader.h>

namespace Gradient
{
    std::unique_ptr<TextureManager> TextureManager::s_textureManager;

    TextureManager::TextureManager()
    {
    }

    void TextureManager::Initialize(ID3D11Device* device, ID3D11DeviceContext* context)
    {
        auto tm = new TextureManager();
        s_textureManager = std::unique_ptr<TextureManager>(tm);
        s_textureManager->LoadWICsRGB(device, context, "default", L"Assets\\DefaultTexture.png");
        s_textureManager->LoadWICLinear(device, context, "defaultNormal", L"Assets\\DefaultNormalMap.png");
        s_textureManager->LoadWICLinear(device, context, "defaultMetalness", L"Assets\\DefaultMetalness.bmp");
    }

    void TextureManager::Shutdown()
    {
        s_textureManager.reset();
    }

    TextureManager* TextureManager::Get()
    {
        return s_textureManager.get();
    }

    void TextureManager::LoadWICsRGB(ID3D11Device* device,
        ID3D11DeviceContext* context,
        std::string key,
        std::wstring path)
    {
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;

        DX::ThrowIfFailed(
            DirectX::CreateWICTextureFromFileEx(device,
                path.c_str(),
                0,
                D3D11_USAGE_DEFAULT,
                D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET,
                0,
                D3D11_RESOURCE_MISC_GENERATE_MIPS,
                DirectX::WIC_LOADER_FORCE_SRGB,
                nullptr,
                srv.ReleaseAndGetAddressOf()));

        m_textureMap.insert({ key, srv });
        context->GenerateMips(srv.Get());
    }

    void TextureManager::LoadWICLinear(ID3D11Device* device,
        ID3D11DeviceContext* context,
        std::string key,
        std::wstring path)
    {
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;

        DX::ThrowIfFailed(
            DirectX::CreateWICTextureFromFileEx(device,
                path.c_str(),
                0,
                D3D11_USAGE_DEFAULT,
                D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET,
                0,
                D3D11_RESOURCE_MISC_GENERATE_MIPS,
                DirectX::WIC_LOADER_IGNORE_SRGB,
                nullptr,
                srv.ReleaseAndGetAddressOf()));

        m_textureMap.insert({ key, srv });
        context->GenerateMips(srv.Get());
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