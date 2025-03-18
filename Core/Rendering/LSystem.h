#pragma once

#include "pch.h"

#include "Core/Rendering/ProceduralMesh.h"
                        
namespace Gradient::Rendering
{
    class LSystem
    {
    public:
        std::unique_ptr<ProceduralMesh> Build(ID3D12Device* device,
            ID3D12CommandQueue* cq);
    };
}