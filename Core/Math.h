#pragma once

#include "pch.h"

#include <directxtk12/SimpleMath.h>
#include <array>

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

    // Makes a plane from three points, facing towards a fourth point
    inline DirectX::SimpleMath::Plane PlaneFromPointsAndSide(const DirectX::SimpleMath::Vector3& point1,
        const DirectX::SimpleMath::Vector3& point2,
        const DirectX::SimpleMath::Vector3& point3,
        const DirectX::SimpleMath::Vector3& pointOnPositiveSide)
    {
        DirectX::SimpleMath::Plane out(point1, point2, point3);

        if (out.DotCoordinate(pointOnPositiveSide) < 0)
        {
            out = -out;
        }

        out.Normalize();
        return out;
    }

    inline std::array<DirectX::XMFLOAT4, 6> GetPlanes(const DirectX::BoundingFrustum& frustum)
    {
        DirectX::SimpleMath::Vector3 corners[8];
        frustum.GetCorners(corners);

        DirectX::SimpleMath::Vector3 centroid = { 0, 0, 0 };
        for (int i = 0; i < 8; i++)
        {
            centroid += corners[i];
        }

        centroid /= 8.f;

        // TODO: Does the far plane come before the near in RH coordinates? I suspect yes
        // Though I suppose it shouldn't matter as long as the planes are all facing the right way
        const size_t bottomLeftNear = 0;
        const size_t bottomRightNear = 1;
        const size_t topRightNear = 2;
        const size_t topLeftNear = 3;
        const size_t bottomLeftFar = 4;
        const size_t bottomRightFar = 5;
        const size_t topRightFar = 6;
        const size_t topLeftFar = 7;

        // Near, Far, Right, Left, Top, Bottom
        std::array<DirectX::XMFLOAT4, 6> out;

        out[0] = PlaneFromPointsAndSide(
            corners[bottomLeftNear],
            corners[bottomRightNear],
            corners[topRightNear],
            centroid);

        out[1] = PlaneFromPointsAndSide(
            corners[bottomLeftFar],
            corners[bottomRightFar],
            corners[topRightFar],
            centroid);

        out[2] = PlaneFromPointsAndSide(
            corners[bottomRightNear],
            corners[bottomRightFar],
            corners[topRightFar],
            centroid);

        out[3] = PlaneFromPointsAndSide(
            corners[bottomLeftNear],
            corners[bottomLeftFar],
            corners[topLeftFar],
            centroid);

        out[4] = PlaneFromPointsAndSide(
            corners[topLeftNear],
            corners[topRightNear],
            corners[topRightFar],
            centroid);

        out[5] = PlaneFromPointsAndSide(
            corners[bottomLeftNear],
            corners[bottomRightNear],
            corners[bottomRightFar],
            centroid);

        return out;
    }

    inline std::array<DirectX::XMFLOAT4, 6> GetPlanes(const DirectX::BoundingOrientedBox& orthoFrustum)
    {
        DirectX::SimpleMath::Vector3 corners[8];
        orthoFrustum.GetCorners(corners);

        // TODO: Does the far plane come before the near in RH coordinates? I suspect yes
        // Though I suppose it shouldn't matter as long as the planes are all facing the right way
        const size_t bottomLeftNear = 0;
        const size_t bottomRightNear = 1;
        const size_t topRightNear = 2;
        const size_t topLeftNear = 3;
        const size_t bottomLeftFar = 4;
        const size_t bottomRightFar = 5;
        const size_t topRightFar = 6;
        const size_t topLeftFar = 7;

        // Near, Far, Right, Left, Top, Bottom
        std::array<DirectX::XMFLOAT4, 6> out;

        out[0] = PlaneFromPointsAndSide(
            corners[bottomLeftNear],
            corners[bottomRightNear],
            corners[topRightNear],
            orthoFrustum.Center);

        out[1] = PlaneFromPointsAndSide(
            corners[bottomLeftFar],
            corners[bottomRightFar],
            corners[topRightFar],
            orthoFrustum.Center);

        out[2] = PlaneFromPointsAndSide(
            corners[bottomRightNear],
            corners[bottomRightFar],
            corners[topRightFar],
            orthoFrustum.Center);

        out[3] = PlaneFromPointsAndSide(
            corners[bottomLeftNear],
            corners[bottomLeftFar],
            corners[topLeftFar],
            orthoFrustum.Center);

        out[4] = PlaneFromPointsAndSide(
            corners[topLeftNear],
            corners[topRightNear],
            corners[topRightFar],
            orthoFrustum.Center);

        out[5] = PlaneFromPointsAndSide(
            corners[bottomLeftNear],
            corners[bottomRightNear],
            corners[bottomRightFar],
            orthoFrustum.Center);

        return out;
    }

    std::vector<DirectX::SimpleMath::Vector2> GeneratePoissonDiskSamples(
        uint32_t numPoints,
        float diskRadius,
        float minDistance);
}