#include "pch.h"

#include "Core/Rendering/LSystem.h"

#include <stack>
#include <sstream>

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
        float Radius = 0.3f;
        float RadiusFactor = 0.7f;
        float AngleDegrees = 25.7f;
        float MoveDistance = 1.5f;

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

        void YawLeft()
        {
            ForwardRotation = Quaternion::Concatenate(ForwardRotation,
                Quaternion::CreateFromAxisAngle(UpVector(),
                    DirectX::XMConvertToRadians(AngleDegrees)));
        }

        void YawRight()
        {
            ForwardRotation = Quaternion::Concatenate(ForwardRotation,
                Quaternion::CreateFromAxisAngle(UpVector(),
                    DirectX::XMConvertToRadians(-AngleDegrees)));
        }

        void RollLeft()
        {
            UpRotation = Quaternion::Concatenate(UpRotation,
                Quaternion::CreateFromAxisAngle(ForwardVector(),
                    DirectX::XMConvertToRadians(-AngleDegrees)));
        }

        void RollRight()
        {
            UpRotation = Quaternion::Concatenate(UpRotation,
                Quaternion::CreateFromAxisAngle(ForwardVector(),
                    DirectX::XMConvertToRadians(AngleDegrees)));
        }

        void PitchUp()
        {
            auto pitchQuat = Quaternion::CreateFromAxisAngle(RightVector(),
                DirectX::XMConvertToRadians(AngleDegrees));

            ForwardRotation = Quaternion::Concatenate(ForwardRotation,
                pitchQuat);
            UpRotation = Quaternion::Concatenate(UpRotation,
                pitchQuat);
        }

        void PitchDown()
        {
            auto pitchQuat = Quaternion::CreateFromAxisAngle(RightVector(),
                DirectX::XMConvertToRadians(-AngleDegrees));

            ForwardRotation = Quaternion::Concatenate(ForwardRotation,
                pitchQuat);
            UpRotation = Quaternion::Concatenate(UpRotation,
                pitchQuat);
        }

        void MoveForward()
        {
            Location += MoveDistance * ForwardVector();
        }
    };

    std::unique_ptr<ProceduralMesh> LSystem::Build(ID3D12Device* device,
        ID3D12CommandQueue* cq)
    {
        std::unordered_map<char, std::string> productionRules;

        productionRules['X'] = "F[+X][-X]FX";
        productionRules['F'] = "FF";

        std::string startingRule = "X";

        std::string rule = ExpandRule(startingRule, productionRules, 3);

        auto tree = InterpretRule(rule);

        return ProceduralMesh::CreateFromPart(device, cq, tree);
    }

    std::string LSystem::ExpandRule(const std::string& startingRule,
        const std::unordered_map<char, std::string>& productionRules,
        int numGenerations)
    {
        std::string previousRule = startingRule;

        for (int i = 0; i < numGenerations; i++)
        {
            std::stringstream stream(previousRule);
            std::ostringstream nextRule;

            while (stream)
            {
                char c;
                stream >> c;

                auto entry = productionRules.find(c);

                if (entry != productionRules.end())
                {
                    nextRule << entry->second;
                }
                else
                {
                    nextRule << c;
                }
            }

            previousRule = nextRule.str();
        }

        return previousRule;
    }

    ProceduralMesh::MeshPart LSystem::InterpretRule(const std::string& rule)
    {
        const int numVerticalSections = 18;

        std::vector<char> ruleCharacters(rule.begin(), rule.end());

        TurtleState turtle;
        std::vector<std::vector<PartParameters>> branches;
        branches.push_back({});
        int branchIndex = 0;

        std::stack<TurtleState> branchStack;

        for (const auto& c : ruleCharacters)
        {
            auto& currentBranch = branches[branchIndex];

            if (c == 'F')
            {
                if (turtle.MoveDistance < 0.2f) continue;

                if (currentBranch.size() > 0)
                {
                    // Update the previous part's top
                    auto lastIndex = currentBranch.size() - 1;

                    Quaternion inverseRotation;
                    currentBranch[lastIndex].BottomRotation.Inverse(inverseRotation);

                    currentBranch[lastIndex].TopRelativeRotation =
                        Quaternion::Concatenate(inverseRotation, turtle.ForwardRotation);
                }

                auto bottomTranslation = turtle.Location;
                auto bottomRotation = turtle.ForwardRotation;
                Quaternion bottomRotationInverse;
                bottomRotation.Inverse(bottomRotationInverse);

                turtle.MoveForward();

                currentBranch.push_back(
                    {
                        bottomTranslation,
                        bottomRotation,
                        turtle.Radius,
                        turtle.Radius,
                        Vector3::Transform(turtle.Location - bottomTranslation,
                            bottomRotationInverse),
                        Quaternion::Concatenate(bottomRotationInverse,
                            turtle.ForwardRotation)
                    }
                );
            }
            else if (c == '[')
            {
                branchStack.push(turtle);
                branches.push_back({});
                branchIndex += 1;
                turtle.Radius *= turtle.RadiusFactor;
            }
            else if (c == ']')
            {
                turtle = branchStack.top();
                turtle.MoveDistance *= 0.8;
                branchIndex -= 1;
                branchStack.pop();
            }
            else if (c == '+')
            {
                turtle.YawLeft();
            }
            else if (c == '-')
            {
                turtle.YawRight();
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

        if (branches[0].size() > 0)
        {
            // Update the previous part's top
            auto lastIndex = branches[0].size() - 1;

            Quaternion inverseRotation;
            branches[0][lastIndex].BottomRotation.Inverse(inverseRotation);

            branches[0][lastIndex].TopRelativeRotation =
                Quaternion::Concatenate(inverseRotation, turtle.ForwardRotation);
        }

        ProceduralMesh::MeshPart tree;

        for (const auto& branch : branches)
        {
            for (const auto& params : branch)
            {
                auto part = ProceduralMesh::CreateAngledFrustumPart(
                    params.BottomRadius,
                    params.TopRadius,
                    params.TopRelativeTranslation,
                    params.TopRelativeRotation,
                    numVerticalSections
                );

                tree.AppendInPlace(part, params.BottomTranslation, params.BottomRotation);
            }
        }

        return tree;
    }
}