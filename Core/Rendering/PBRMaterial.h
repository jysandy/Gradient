#pragma once

#include "pch.h"
#include "Core/GraphicsMemoryManager.h"

#include <directxtk12/SimpleMath.h>

namespace Gradient::Rendering
{
    struct PBRMaterial
    {
        GraphicsMemoryManager::DescriptorView Texture = nullptr;
        GraphicsMemoryManager::DescriptorView NormalMap = nullptr;
        GraphicsMemoryManager::DescriptorView AOMap = nullptr;
        GraphicsMemoryManager::DescriptorView MetalnessMap = nullptr;
        GraphicsMemoryManager::DescriptorView RoughnessMap = nullptr;
        DirectX::SimpleMath::Vector3 EmissiveRadiance = { 0, 0, 0 };

        PBRMaterial() = default;
        PBRMaterial(std::string albedoKey,
            std::string normalKey,
            std::string aoKey,
            std::string metallicKey,
            std::string roughnessKey);

        static PBRMaterial DefaultPBRMaterial();
    };
}