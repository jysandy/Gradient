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

        std::unique_ptr<ProceduralMesh> Build(ID3D12Device* device,
            ID3D12CommandQueue* cq);

    private:
        ProceduralMesh::MeshPart InterpretRule(const std::string& rule);
        std::string ExpandRule(const std::string& startingRule,
            const std::unordered_map<char, std::string>& productionRules,
            int numGenerations);
    };
}