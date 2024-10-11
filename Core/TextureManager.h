#pragma once

#include "pch.h"

#include <unordered_map>

namespace Gradient
{
    class TextureManager
    {
    public:
        static void Initialize(ID3D11Device* device);
        static void Shutdown();

        static TextureManager* Get();
        void LoadWICTexture(ID3D11Device* device, std::string key, std::wstring path);
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetTexture(std::string key);

    private:
        static std::unique_ptr<TextureManager> s_textureManager;

        TextureManager();



        std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> m_textureMap;
    };
}