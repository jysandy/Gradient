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
        this->Primitive.reset();
    }

    void Entity::SetRotation(float yaw, float pitch, float roll)
    {
        if (this->BodyID.IsInvalid())
        {
            Rotation = Matrix::CreateFromYawPitchRoll(yaw, pitch, roll);
        }
        else
        {
            auto& bodyInterface = Gradient::Physics::PhysicsEngine::Get()->GetBodyInterface();
            bool isActive = bodyInterface.IsActive(BodyID);
            auto rotationQuat = Quaternion::CreateFromYawPitchRoll(yaw, pitch, roll);
            bodyInterface.SetRotation(BodyID, JPH::Quat(
                rotationQuat.x,
                rotationQuat.y,
                rotationQuat.z,
                rotationQuat.w),
                isActive ? JPH::EActivation::Activate : JPH::EActivation::DontActivate
            );

            // Static entities are not synced by the EntityManager
            if (bodyInterface.GetMotionType(BodyID) == JPH::EMotionType::Static)
            {
                Rotation = Matrix::CreateFromQuaternion(rotationQuat);
            }
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
        if (this->BodyID.IsInvalid())
        {
            this->Translation = Matrix::CreateTranslation(offset);
        }
        else
        {
            auto& bodyInterface = Gradient::Physics::PhysicsEngine::Get()->GetBodyInterface();
            bool isActive = bodyInterface.IsActive(BodyID);
            bodyInterface.SetPosition(BodyID,
                JPH::Vec3(offset.x, offset.y, offset.z),
                isActive ? JPH::EActivation::Activate : JPH::EActivation::DontActivate
                );

            // Static entities are not synced by the EntityManager
            if (bodyInterface.GetMotionType(BodyID) == JPH::EMotionType::Static)
            {
                Translation = Matrix::CreateTranslation(offset);
            }
        }
    }
}