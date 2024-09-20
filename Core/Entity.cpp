#include "pch.h"
#include "Core/Entity.h"

using namespace DirectX::SimpleMath;

namespace Gradient
{
    Entity::Entity()
    {
        this->Scale = Matrix::Identity;
        this->Rotation = Matrix::Identity;
        this->Translation = Matrix::Identity;
    }

    Matrix Entity::GetWorldMatrix()
    {
        return this->Scale * this->Rotation * this->Translation;
    }

    void Entity::OnDeviceLost()
    {
        this->Primitive.reset();
    }
}