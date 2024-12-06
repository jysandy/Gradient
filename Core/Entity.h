#pragma once

#include <Jolt/Physics/Body/BodyID.h>

#include <directxtk/SimpleMath.h>
#include <directxtk/GeometricPrimitive.h>
#include <memory>
#include <string>
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
        Pipelines::IRenderPipeline* ShadowPipeline;
        bool CastsShadows = true;

        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Texture = nullptr;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> NormalMap = nullptr;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> AOMap = nullptr;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> MetalnessMap = nullptr;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> RoughnessMap = nullptr;
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
