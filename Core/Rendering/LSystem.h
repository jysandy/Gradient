#pragma once

#include "pch.h"

#include "Core/Rendering/ProceduralMesh.h"
                        
namespace Gradient::Rendering
{
    class LSystem
    {
    public:
        struct LeafTransform
        {
            DirectX::SimpleMath::Vector3 Translation;
            DirectX::SimpleMath::Quaternion Rotation;
        };

        void AddRule(char lhs, const std::string& rhs);

        void Build(std::string startingRule,
            int numGenerations,
            int numVerticalSections = 6);

        const ProceduralMesh::MeshPart& GetTrunk() const;

        void Combine(const LSystem& subsystem);
        std::vector<LeafTransform> GetCombinedLeaves(const LSystem& subsystem) const;

        const std::vector<LeafTransform>& GetLeafTransforms() const;

        float StartingRadius = 0.3f;
        float RadiusFactor = 0.7f;
        float AngleDegrees = 25.7f;
        float MoveDistance = 1.f;

    private:
        bool m_isBuilt = false;

        ProceduralMesh::MeshPart InterpretRule(const std::string& rule, int numVerticalSections);
        std::string ExpandRule(const std::string& startingRule,
            const std::unordered_map<char, std::string>& productionRules,
            int numGenerations);

        std::unordered_map<char, std::string> m_productionRules;

        ProceduralMesh::MeshPart m_trunkPart;
        std::vector<LeafTransform> m_leafTransforms;
    };
}