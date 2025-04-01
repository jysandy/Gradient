#pragma once

#include "pch.h"

#include "Core/Rendering/ProceduralMesh.h"
                        
namespace Gradient::Rendering
{
    class LSystem
    {
    public:
        std::unique_ptr<ProceduralMesh> BuildTwo(ID3D12Device* device,
            ID3D12CommandQueue* cq);

        void AddRule(char lhs, const std::string& rhs);

        std::unique_ptr<ProceduralMesh> Build(ID3D12Device* device,
            ID3D12CommandQueue* cq,
            std::string startingRule,
            int numGenerations);

        const std::vector<DirectX::SimpleMath::Matrix>& GetLeafTransforms() const;

    private:
        ProceduralMesh::MeshPart InterpretRule(const std::string& rule);
        std::string ExpandRule(const std::string& startingRule,
            const std::unordered_map<char, std::string>& productionRules,
            int numGenerations);

        std::unordered_map<char, std::string> m_productionRules;

        std::vector<DirectX::SimpleMath::Matrix> m_leafTransforms;
    };
}