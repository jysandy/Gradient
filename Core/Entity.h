#pragma once

#include <Jolt/Physics/Body/BodyID.h>

#include <directxtk/SimpleMath.h>
#include <directxtk/GeometricPrimitive.h>
#include <memory>
#include <string>

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
        std::unique_ptr<DirectX::GeometricPrimitive> Primitive;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Texture = nullptr;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> NormalMap = nullptr;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> AOMap = nullptr;

        Entity();
        DirectX::SimpleMath::Matrix GetWorldMatrix() const;
        void OnDeviceLost();

        DirectX::SimpleMath::Vector3 GetRotationYawPitchRoll() const;
        void SetRotation(float yaw, float pitch, float roll);
        DirectX::SimpleMath::Vector3 GetTranslation() const;
        void SetTranslation(const DirectX::SimpleMath::Vector3& offset);
    };
}
