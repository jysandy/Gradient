#pragma once

#include "pch.h"

#include <directxtk12/SimpleMath.h>

namespace Gradient::Math
{
    inline DirectX::BoundingFrustum MakeFrustum(
        DirectX::SimpleMath::Matrix view,
        DirectX::SimpleMath::Matrix proj)
    {
        DirectX::BoundingFrustum viewSpaceFrustum;

        DirectX::BoundingFrustum::CreateFromMatrix(viewSpaceFrustum,
            proj,
            true);

        DirectX::SimpleMath::Matrix inverseViewMatrix;
        view.Invert(inverseViewMatrix);

        DirectX::BoundingFrustum worldSpaceFrustum;
        viewSpaceFrustum.Transform(worldSpaceFrustum, inverseViewMatrix);

        return worldSpaceFrustum;
    }

    std::vector<DirectX::SimpleMath::Vector2> GeneratePoissonDiskSamples(
        uint32_t numPoints,
        float diskRadius,
        float minDistance);
}