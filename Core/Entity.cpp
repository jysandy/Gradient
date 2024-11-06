#include "pch.h"
#include "Core/Entity.h"
#include "Core/Physics/PhysicsEngine.h"

using namespace DirectX::SimpleMath;

namespace Gradient
{
    Entity::Entity()
    {
        this->Scale = Matrix::Identity;
        this->Rotation = Matrix::Identity;
        this->Translation = Matrix::Identity;
    }

    Matrix Entity::GetWorldMatrix() const
    {
        return this->Scale * this->Rotation * this->Translation;
    }

    void Entity::OnDeviceLost()
    {
        this->Drawable.reset();
    }

    void Entity::SetRotation(float yaw, float pitch, float roll)
    {
        Rotation = Matrix::CreateFromYawPitchRoll(yaw, pitch, roll);
     
        if (!this->BodyID.IsInvalid())
        {
            auto& bodyInterface = Gradient::Physics::PhysicsEngine::Get()->GetBodyInterface();
            auto rotationQuat = Quaternion::CreateFromYawPitchRoll(yaw, pitch, roll);
            bodyInterface.SetRotation(BodyID, JPH::Quat(
                rotationQuat.x,
                rotationQuat.y,
                rotationQuat.z,
                rotationQuat.w),
                JPH::EActivation::Activate
            );
        }
    }

    Vector3 Entity::GetRotationYawPitchRoll() const
    {
        // This doesn't read from the physics engine, for efficiency
        return Rotation.ToEuler();
    }

    Vector3 Entity::GetTranslation() const
    {
        // This doesn't read from the physics engine, for efficiency
        return this->Translation.Translation();
    }

    void Entity::SetTranslation(const Vector3& offset)
    {
        this->Translation = Matrix::CreateTranslation(offset);

        if (!this->BodyID.IsInvalid())
        {
            auto& bodyInterface = Gradient::Physics::PhysicsEngine::Get()->GetBodyInterface();
            bodyInterface.SetPosition(BodyID,
                JPH::Vec3(offset.x, offset.y, offset.z),
                JPH::EActivation::Activate
            );
        }
    }
}