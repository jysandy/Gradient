#pragma once

#include "pch.h"

#include <Jolt/Math/Math.h>
#include <directxtk12/SimpleMath.h>

namespace Gradient::Physics
{
    inline JPH::RVec3 ToJolt(DirectX::SimpleMath::Vector3 in)
    {
        return JPH::RVec3(in.x, in.y, in.z);
    }

    inline DirectX::SimpleMath::Vector3 FromJolt(JPH::RVec3 in)
    {
        return DirectX::SimpleMath::Vector3(
            in.GetX(),
            in.GetY(),
            in.GetZ());
    }

    inline DirectX::SimpleMath::Color FromJolt(JPH::Color in)
    {
        return DirectX::SimpleMath::Color(
            in.r,
            in.g,
            in.b,
            in.a);
    }
}