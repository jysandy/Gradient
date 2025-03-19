#include "pch.h"

#include "Core/Rendering/LSystem.h"

using namespace DirectX::SimpleMath;

namespace Gradient::Rendering
{
    std::unique_ptr<ProceduralMesh> LSystem::BuildTwo(ID3D12Device* device,
        ID3D12CommandQueue* cq)
    {
        using namespace DirectX::SimpleMath;

        const int numVerticalSections = 18;

        Quaternion bottomRotation = Quaternion::CreateFromYawPitchRoll(
            { 0, 0, -DirectX::XM_PIDIV4 });

        Vector3 bottomEnd = { 0, 5, 0 };

        ProceduralMesh::MeshPart bottom = ProceduralMesh::CreateAngledFrustumPart(
            2.f, 1.5f, bottomEnd, bottomRotation, numVerticalSections
        );

        Quaternion topRotation = Quaternion::CreateFromYawPitchRoll(
            { 0, 0, -DirectX::XM_PI / 6.f });

        ProceduralMesh::MeshPart top = ProceduralMesh::CreateAngledFrustumPart(
            1.5f, 1.5f, { 0, 5, 0 }, topRotation, numVerticalSections
        );

        ProceduralMesh::MeshPart system =
            bottom.Append(top, bottomEnd, bottomRotation);

        ProceduralMesh::MeshPart top2 = ProceduralMesh::CreateAngledFrustumPart(
            1.5f, 1.5f, { 0, 5, 0 }, topRotation, numVerticalSections
        );

        system = system.Append(top2, { 0, 10, 0 },
            Quaternion::Concatenate(bottomRotation, topRotation));

        return ProceduralMesh::CreateFromPart(device, cq, system);
    }

    struct PartParameters
    {
        Vector3 BottomTranslation;
        Quaternion BottomRotation;
        float BottomRadius;
        float TopRadius;
        Vector3 TopRelativeTranslation;
        Quaternion TopRelativeRotation;
    };

    struct TurtleState
    {
        Vector3 Location = { 0, 0, 0 };
        Quaternion ForwardRotation = Quaternion::Identity;
        Quaternion UpRotation = Quaternion::Identity;

        Vector3 ForwardVector()
        {
            return Vector3::Transform(Vector3::UnitY, ForwardRotation);
        }

        Vector3 UpVector()
        {
            return Vector3::Transform(Vector3::UnitZ, UpRotation);
        }

        Vector3 RightVector()
        {
            return ForwardVector().Cross(UpVector());
        }


        void RollLeft()
        {
            UpRotation = Quaternion::Concatenate(UpRotation,
                Quaternion::CreateFromAxisAngle(ForwardVector(),
                    DirectX::XMConvertToRadians(-10.f)));
        }

        void RollRight()
        {
            UpRotation = Quaternion::Concatenate(UpRotation,
                Quaternion::CreateFromAxisAngle(ForwardVector(),
                    DirectX::XMConvertToRadians(10.f)));
        }

        void PitchUp()
        {
            auto pitchQuat = Quaternion::CreateFromAxisAngle(RightVector(),
                DirectX::XMConvertToRadians(10.f));

            ForwardRotation = Quaternion::Concatenate(ForwardRotation,
                pitchQuat);
            UpRotation = Quaternion::Concatenate(UpRotation,
                pitchQuat);
        }

        void PitchDown()
        {
            auto pitchQuat = Quaternion::CreateFromAxisAngle(RightVector(),
                DirectX::XMConvertToRadians(-10.f));

            ForwardRotation = Quaternion::Concatenate(ForwardRotation,
                pitchQuat);
            UpRotation = Quaternion::Concatenate(UpRotation,
                pitchQuat);
        }

        void MoveForward()
        {
            Location += ForwardVector();
        }
    };

    std::unique_ptr<ProceduralMesh> LSystem::Build(ID3D12Device* device,
        ID3D12CommandQueue* cq)
    {
        const int numVerticalSections = 18;

        std::string rule = "F//&F\\^F\\\\/&&F^^^//^^F//^^F\\&&F/&/&F";

        std::vector<char> ruleCharacters(rule.begin(), rule.end());

        TurtleState turtle;
        std::vector<PartParameters> partParameters;

        const float radius = 0.5f;

        for (const auto& c : ruleCharacters)
        {
            if (c == 'F')
            {
                if (partParameters.size() > 0)
                {
                    // Update the previous part's top
                    auto lastIndex = partParameters.size() - 1;

                    Quaternion inverseRotation;
                    partParameters[lastIndex].BottomRotation.Inverse(inverseRotation);

                    partParameters[lastIndex].TopRelativeRotation =
                        Quaternion::Concatenate(inverseRotation, turtle.ForwardRotation);
                }

                auto bottomTranslation = turtle.Location;
                auto bottomRotation = turtle.ForwardRotation;
                Quaternion bottomRotationInverse;
                bottomRotation.Inverse(bottomRotationInverse);

                turtle.MoveForward();

                partParameters.push_back(
                    {
                        bottomTranslation,
                        bottomRotation,
                        radius,
                        radius,
                        Vector3::Transform(turtle.Location - bottomTranslation,
                            bottomRotationInverse),
                        Quaternion::Concatenate(bottomRotationInverse,
                            turtle.ForwardRotation)
                    }
                );
            }
            else if (c == '/')
            {
                turtle.RollRight();
            }
            else if (c == '\\')
            {
                turtle.RollLeft();
            }
            else if (c == '^')
            {
                turtle.PitchUp();
            }
            else if (c == '&')
            {
                turtle.PitchDown();
            }
        }

        if (partParameters.size() > 0)
        {
            // Update the previous part's top
            auto lastIndex = partParameters.size() - 1;

            Quaternion inverseRotation;
            partParameters[lastIndex].BottomRotation.Inverse(inverseRotation);

            partParameters[lastIndex].TopRelativeRotation =
                Quaternion::Concatenate(inverseRotation, turtle.ForwardRotation);
        }

        ProceduralMesh::MeshPart tree;

        for (const auto& params : partParameters)
        {
            auto part = ProceduralMesh::CreateAngledFrustumPart(
                radius,
                radius,
                params.TopRelativeTranslation,
                params.TopRelativeRotation,
                numVerticalSections
            );

            tree = tree.Append(part, params.BottomTranslation, params.BottomRotation);
        }

        return ProceduralMesh::CreateFromPart(device, cq, tree);
    }
}