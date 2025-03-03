#pragma once

#include "pch.h"

#include <Jolt/Renderer/DebugRendererSimple.h>
#include <Jolt/Core/StringTools.h>

#include <directxtk12/Effects.h>
#include <directxtk12/VertexTypes.h>
#include <directxtk12/PrimitiveBatch.h>
#include <directxtk12/SimpleMath.h>

namespace Gradient::Physics
{
    class DebugRenderer : public JPH::DebugRendererSimple
    {
    public:
        using VertexType = DirectX::VertexPositionColor;

        DebugRenderer(ID3D12Device* device,
            DXGI_FORMAT renderTargetFormat,
            DXGI_FORMAT depthBufferformat = DXGI_FORMAT_D32_FLOAT);

        void SetFrameVariables(ID3D12GraphicsCommandList* cl,
            DirectX::SimpleMath::Matrix viewMatrix,
            DirectX::SimpleMath::Matrix projectionMatrix,
            DirectX::SimpleMath::Vector3 cameraPos);

        virtual void DrawLine(JPH::RVec3Arg inFrom,
            JPH::RVec3Arg inTo,
            JPH::ColorArg inColor) override;

        virtual void DrawTriangle(JPH::RVec3Arg inV1,
            JPH::RVec3Arg inV2,
            JPH::RVec3Arg inV3,
            JPH::ColorArg inColor,
            ECastShadow inCastShadow) override;

        virtual void DrawText3D(JPH::RVec3Arg inPosition,
            const JPH::string_view& inString,
            JPH::ColorArg inColor,
            float inHeight) override;

    private:
        std::unique_ptr<DirectX::BasicEffect> m_triangleEffect;
        std::unique_ptr<DirectX::BasicEffect> m_lineEffect;
        std::unique_ptr<DirectX::PrimitiveBatch<VertexType>> m_batch;
        ID3D12GraphicsCommandList* m_cl;
    };
}