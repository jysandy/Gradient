#pragma once

#include <directxtk/SimpleMath.h>
#include <array>

namespace Gradient::Params
{
    struct WaterScattering
    {
        float ThicknessPower = 3.f;
        float Sharpness = 4.f;
        float RefractiveIndex = 1.5f;
    };

    struct Water
    {
        float MinLod = 50.f;
        float MaxLod = 400.f;
        WaterScattering Scattering;
    };

    struct PointLight
    {
        DirectX::SimpleMath::Color Colour = { 1.f, 1.f, 1.f, 1.f };
        float Irradiance = 3.f;
        DirectX::SimpleMath::Vector3 Position = { 0.f, 0.f, 0.f };
        float MinRange = 0.1f;
        float MaxRange = 30.f;
        uint32_t ShadowCubeIndex = 0.f;

        std::array<DirectX::SimpleMath::Matrix, 6> GetViewMatrices()
        {
            using namespace DirectX::SimpleMath;
            std::array<DirectX::SimpleMath::Matrix, 6> ret;
            
            // +X
            // -X
            // +Y
            // -Y
            // +Z
            // -Z
            Vector3 lookAt[6] = {
                Vector3::UnitX,
                -Vector3::UnitX,
                Vector3::UnitY,
                -Vector3::UnitY,
                Vector3::UnitZ,
                -Vector3::UnitZ
            };
            Vector3 up[6] = {
               Vector3::UnitY,
               Vector3::UnitY,
               Vector3::UnitZ,
               -Vector3::UnitZ,
               Vector3::UnitY,
               Vector3::UnitY
            };

            for (int i = 0; i < 6; i++)
            {
                auto look = lookAt[i];

                ret[i] = Matrix::CreateLookAt(Position,
                    Position + look, up[i]);
            }

            return ret;
        }

        DirectX::SimpleMath::Matrix GetProjectionMatrix()
        {
            using namespace DirectX::SimpleMath;

            return Matrix::CreatePerspectiveFieldOfView(
                DirectX::XM_PIDIV2,
                1.f, MinRange, MaxRange);
        }
    };
}

