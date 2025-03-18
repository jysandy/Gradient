#include "pch.h"

#include "Core/Rendering/LSystem.h"

namespace Gradient::Rendering
{
    std::unique_ptr<ProceduralMesh> LSystem::Build(ID3D12Device* device,
        ID3D12CommandQueue* cq)
    {
        using namespace DirectX::SimpleMath;

        const int numVerticalSections = 18;

        Quaternion bottomRotation = Quaternion::CreateFromYawPitchRoll(
            { 0, 0, -DirectX::XM_PIDIV4 });

        Vector3 bottomEnd = { 2, 5, 0 };

        ProceduralMesh::MeshPart bottom = ProceduralMesh::CreateAngledFrustumPart(
            2.f, 1.5f, bottomEnd, bottomRotation, numVerticalSections
        );

        Quaternion topRotation = Quaternion::CreateFromYawPitchRoll(
            { 0, 0, -DirectX::XM_PIDIV2 });

        topRotation = Quaternion::Identity;

        ProceduralMesh::MeshPart top = ProceduralMesh::CreateAngledFrustumPart(
            1.5f, 1.5f, { 0, 5, 0 }, topRotation, numVerticalSections
        );

        ProceduralMesh::MeshPart system =
            bottom.Append(top, bottomEnd, bottomRotation);

        return ProceduralMesh::CreateFromPart(device, cq, system);
    }
}