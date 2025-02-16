#include "pch.h"

#include "Core/Rendering/PBRMaterial.h"
#include "Core/TextureManager.h"

namespace Gradient::Rendering
{
    PBRMaterial PBRMaterial::DefaultPBRMaterial()
    {
        auto tm = TextureManager::Get();

        auto blankTexture = tm->GetTexture("default");
        auto outwardNormalMap = tm->GetTexture("defaultNormal");
        auto dielectricMetalnessMap = tm->GetTexture("defaultMetalness");
        auto smoothMap = dielectricMetalnessMap;

        PBRMaterial material;

        material.Texture = blankTexture;
        material.NormalMap = outwardNormalMap;
        material.AOMap = blankTexture;
        material.MetalnessMap = dielectricMetalnessMap;
        material.RoughnessMap = blankTexture;
        material.EmissiveRadiance = { 0, 0, 0 };

        return material;
    }

    PBRMaterial::PBRMaterial(std::string albedoKey,
        std::string normalKey,
        std::string aoKey,
        std::string metallicKey,
        std::string roughnessKey)
    {
        auto tm = TextureManager::Get();

        Texture = tm->GetTexture(albedoKey);
        NormalMap = tm->GetTexture(normalKey);
        AOMap = tm->GetTexture(aoKey);
        MetalnessMap = tm->GetTexture(metallicKey);
        RoughnessMap = tm->GetTexture(roughnessKey);
    }
}