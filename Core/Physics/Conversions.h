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
}