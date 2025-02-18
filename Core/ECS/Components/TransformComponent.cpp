#include "pch.h"

#include "Core/ECS/Components/TransformComponent.h"

namespace Gradient::ECS::Components
{
    DirectX::SimpleMath::Matrix TransformComponent::GetWorldMatrix() const
    {
        return this->Scale * this->Rotation * this->Translation;
    }

    DirectX::SimpleMath::Vector3 TransformComponent::GetRotationYawPitchRoll() const
    {
        return Rotation.ToEuler();
    }

    DirectX::SimpleMath::Vector3 TransformComponent::GetTranslation() const
    {
        return Translation.Translation();
    }
}