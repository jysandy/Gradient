#include "pch.h"

#include "Core/Physics/DebugRenderer.h"
#include "Core/Physics/Conversions.h"
#include <directxtk12/CommonStates.h>

namespace Gradient::Physics
{
    DebugRenderer::DebugRenderer(ID3D12Device* device,
        DXGI_FORMAT renderTargetFormat,
        DXGI_FORMAT depthBufferFormat)
    {
        m_batch = std::make_unique<DirectX::PrimitiveBatch<VertexType>>(device);

        DirectX::RenderTargetState rtState(renderTargetFormat, depthBufferFormat);
        rtState.sampleDesc = DXGI_SAMPLE_DESC{ 4, 0 }; // TODO: pass the sample desc through

        DirectX::EffectPipelineStateDescription pd(
            &VertexType::InputLayout,
            DirectX::CommonStates::Opaque,
            DirectX::CommonStates::DepthDefault,
            DirectX::CommonStates::CullNone,
            rtState
        );

        m_triangleEffect = std::make_unique<DirectX::BasicEffect>(device, 
            DirectX::EffectFlags::VertexColor, 
            pd);

        pd = DirectX::EffectPipelineStateDescription(
            &VertexType::InputLayout,
            DirectX::CommonStates::Opaque,
            DirectX::CommonStates::DepthDefault,
            DirectX::CommonStates::CullNone,
            rtState,
            D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE);

        m_lineEffect = std::make_unique<DirectX::BasicEffect>(device,
            DirectX::EffectFlags::VertexColor,
            pd);
    }

    void DebugRenderer::SetFrameVariables(ID3D12GraphicsCommandList* cl,
        DirectX::SimpleMath::Matrix viewMatrix,
        DirectX::SimpleMath::Matrix projectionMatrix,
        DirectX::SimpleMath::Vector3 cameraPos)
    {
        m_cl = cl;

        this->SetCameraPos(ToJolt(cameraPos));

        m_triangleEffect->SetView(viewMatrix);
        m_triangleEffect->SetProjection(projectionMatrix);
        m_triangleEffect->SetWorld(DirectX::SimpleMath::Matrix::Identity);

        m_lineEffect->SetView(viewMatrix);
        m_lineEffect->SetProjection(projectionMatrix);
        m_lineEffect->SetWorld(DirectX::SimpleMath::Matrix::Identity);
    }

    void DebugRenderer::DrawLine(JPH::RVec3Arg inFrom,
        JPH::RVec3Arg inTo,
        JPH::ColorArg inColor)
    {
        VertexType v1(FromJolt(inFrom), FromJolt(inColor));
        VertexType v2(FromJolt(inTo), FromJolt(inColor));
        
        m_lineEffect->Apply(m_cl);
        m_batch->Begin(m_cl);
        m_batch->DrawLine(v1, v2);
        m_batch->End();
    }

    void DebugRenderer::DrawTriangle(JPH::RVec3Arg inV1,
        JPH::RVec3Arg inV2,
        JPH::RVec3Arg inV3,
        JPH::ColorArg inColor,
        ECastShadow inCastShadow)
    {
        UNREFERENCED_PARAMETER(inCastShadow);

        m_triangleEffect->Apply(m_cl);
        m_batch->Begin(m_cl);

        VertexType v1(FromJolt(inV1), FromJolt(inColor));
        VertexType v2(FromJolt(inV2), FromJolt(inColor));
        VertexType v3(FromJolt(inV3), FromJolt(inColor));

        // This is too slow!
        // TODO: Implement JPH::DebugRenderer completely
        m_batch->DrawTriangle(v1, v2, v3);
        m_batch->End();
    }

    void DebugRenderer::DrawText3D(JPH::RVec3Arg inPosition,
        const JPH::string_view& inString,
        JPH::ColorArg inColor,
        float inHeight) 
    {
        // Does nothing for now
    }
}