#include "pch.h"

#include "Core/Rendering/LSystem.h"

#include <stack>
#include <sstream>

using namespace DirectX::SimpleMath;

namespace Gradient::Rendering
{
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
        float MoveDistance = 1.f;

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

    void LSystem::AddRule(char lhs, const std::string& rhs)
    {
        m_productionRules[lhs] = rhs;
    }

    void LSystem::Build(std::string startingRule,
        int numGenerations)
    {
        if (m_isBuilt) return;

        std::string rule = ExpandRule(startingRule,
            m_productionRules,
            numGenerations);

        m_trunkPart = InterpretRule(rule);
        m_isBuilt = true;
    }

    const ProceduralMesh::MeshPart& LSystem::GetTrunk() const
    {
        return m_trunkPart;
    }

    void LSystem::Combine(const LSystem& subsystem)
    {
        std::vector<LSystem::LeafTransform> newLeafTransforms;

        for (const auto& trunkTransform : m_leafTransforms)
        {
            m_trunkPart.AppendInPlace(subsystem.GetTrunk(),
                trunkTransform.Translation,
                trunkTransform.Rotation);

            for (const auto& subsystemLeaf : subsystem.GetLeafTransforms())
            {
                newLeafTransforms.push_back(
                    {
                        trunkTransform.Translation
                            + Vector3::Transform(subsystemLeaf.Translation,
                                trunkTransform.Rotation),
                        Quaternion::Concatenate(trunkTransform.Rotation,
                            subsystemLeaf.Rotation)
                    });
            }
        }

        m_leafTransforms = newLeafTransforms;
    }

    std::unique_ptr<ProceduralMesh> LSystem::GetMesh(ID3D12Device* device,
        ID3D12CommandQueue* cq,
        std::string startingRule,
        int numGenerations)
    {
        Build(startingRule, numGenerations);

        return ProceduralMesh::CreateFromPart(device, cq, m_trunkPart);
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

            for (int j = 0; j < previousRule.size(); j++)
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
        const int numVerticalSections = 6;

        std::vector<char> ruleCharacters(rule.begin(), rule.end());

        TurtleState turtle;

        turtle.Radius = StartingRadius;
        turtle.RadiusFactor = RadiusFactor;
        turtle.AngleDegrees = AngleDegrees;
        turtle.MoveDistance = MoveDistance;

        std::vector<std::vector<PartParameters>> branches;
        branches.push_back({});
        int branchIndex = 0;

        struct StackState {
            TurtleState Turtle;
            int BranchIndex;
        };

        std::stack<StackState> branchStack;

        for (const auto& c : ruleCharacters)
        {
            auto& currentBranch = branches[branchIndex];

            if (c == 'F')
            {
                //if (turtle.MoveDistance < 0.3f) continue;

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
            else if (c == 'L')
            {
                m_leafTransforms.push_back({turtle.Location, turtle.ForwardRotation});
            }
            else if (c == '[')
            {
                branchStack.push({ turtle, branchIndex });
                branchIndex = branches.size();
                branches.push_back({});
                turtle.Radius *= turtle.RadiusFactor;
            }
            else if (c == ']')
            {
                auto state = branchStack.top();
                turtle = state.Turtle;
                //turtle.MoveDistance *= 0.8;
                branchIndex = state.BranchIndex;
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

    const std::vector<LSystem::LeafTransform>& LSystem::GetLeafTransforms() const
    {
        return m_leafTransforms;
    }
}