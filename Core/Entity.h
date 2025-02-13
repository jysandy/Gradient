#pragma once

#include <Jolt/Physics/Body/BodyID.h>

#include <directxtk12/SimpleMath.h>
#include <memory>
#include <string>
#include <optional>
#include "Core/GraphicsMemoryManager.h"
#include "Core/Rendering/IDrawable.h"
#include "Core/Pipelines/IRenderPipeline.h"

namespace Gradient
{
    struct Entity
    {
        std::string id;
        JPH::BodyID BodyID;
        // TODO: Add a transform for the physics body

        DirectX::SimpleMath::Matrix Scale;
        DirectX::SimpleMath::Matrix Rotation;
        DirectX::SimpleMath::Matrix Translation;
        std::unique_ptr<Rendering::IDrawable> Drawable;
        Pipelines::IRenderPipeline* RenderPipeline;
        bool CastsShadows = true;

        GraphicsMemoryManager::DescriptorView Texture = nullptr;
        GraphicsMemoryManager::DescriptorView NormalMap = nullptr;
        GraphicsMemoryManager::DescriptorView AOMap = nullptr;
        GraphicsMemoryManager::DescriptorView MetalnessMap = nullptr;
        GraphicsMemoryManager::DescriptorView RoughnessMap = nullptr;
        DirectX::SimpleMath::Vector3 EmissiveRadiance = { 0, 0, 0 };

        Entity();
        DirectX::SimpleMath::Matrix GetWorldMatrix() const;
        void OnDeviceLost();

        DirectX::SimpleMath::Vector3 GetRotationYawPitchRoll() const;
        void SetRotation(float yaw, float pitch, float roll);
        DirectX::SimpleMath::Vector3 GetTranslation() const;
        void SetTranslation(const DirectX::SimpleMath::Vector3& offset);
    };
}
