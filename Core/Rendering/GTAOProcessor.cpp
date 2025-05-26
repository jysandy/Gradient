#include "pch.h"

#include "Core/Rendering/GTAOProcessor.h"
#include "Core/GraphicsMemoryManager.h"
#include "Core/ReadData.h"
#include "Core/Math.h"

namespace Gradient::Rendering
{
    Microsoft::WRL::ComPtr<ID3D12PipelineState> CreateComputePipelineState(
        ID3D12Device* device,
        const std::wstring& shaderPath,
        ID3D12RootSignature* rootSignature
    )
    {
        Microsoft::WRL::ComPtr<ID3D12PipelineState> out;

        auto csData = DX::ReadData(shaderPath.c_str());
        D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = rootSignature;
        psoDesc.CS = { csData.data(), csData.size() };
        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(out.ReleaseAndGetAddressOf())));

        return out;
    }

    GTAOProcessor::GTAOProcessor(ID3D12Device* device,
        RECT windowSize)
    {
        m_gtaoSettings.DenoisePasses = 1;

        auto gmm = GraphicsMemoryManager::Get();

        // --- Raw, single sampled input depth buffer

        auto inputDepthBufferDesc = CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_R32_TYPELESS,
            windowSize.right,
            windowSize.bottom,
            1, 1
        );

        m_inputDepthBuffer.Create(device,
            &inputDepthBufferDesc,
            D3D12_RESOURCE_STATE_RESOLVE_DEST,
            nullptr);

        m_inputDepthBuffer.Get()->SetName(L"GTAO Input depth buffer");

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        m_inputDepthBufferSRV = gmm->CreateSRV(device, m_inputDepthBuffer.Get(), &srvDesc);

        // --- Normals

        auto normalsDesc = CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_R32_UINT,
            windowSize.right,
            windowSize.bottom,
            1, 1
        );
        normalsDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        m_normals.Create(device,
            &normalsDesc,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            nullptr);
        m_normals.Get()->SetName(L"GTAO Generated Normals");
        m_normalsUAV = gmm->CreateUAV(device, m_normals.Get());
        m_normalsSRV = gmm->CreateSRV(device, m_normals.Get());

        // --- Normals root signature and PSO

        m_generateNormalsRS.AddCBV(0, 0); // constants
        m_generateNormalsRS.AddSRV(0, 0); // raw depth
        m_generateNormalsRS.AddUAV(0, 0); // normals output

        m_generateNormalsRS.AddStaticSampler(CD3DX12_STATIC_SAMPLER_DESC(1,
            D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP),
            0, 0);

        m_generateNormalsRS.Build(device, true);

        m_generateNormalsPSO = CreateComputePipelineState(device,
            L"GTAONormals_CS.cso",
            m_generateNormalsRS.Get());

        // --- Working depths

        auto workingDepthsDesc = CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_R32_FLOAT,
            windowSize.right,
            windowSize.bottom,
            1, XE_GTAO_DEPTH_MIP_LEVELS
        );
        workingDepthsDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        m_workingDepths.Create(device,
            &workingDepthsDesc,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            nullptr);
        m_workingDepths.Get()->SetName(L"GTAO Working Depths");

        m_workingDepthsSRV = gmm->CreateSRV(device, m_workingDepths.Get());

        for (int i = 0; i < XE_GTAO_DEPTH_MIP_LEVELS; i++)
        {
            m_workingDepthsUAVs[i] = gmm->CreateUAV(device, m_workingDepths.Get(), i);
        }

        // --- Prefilter depths root signature and PSO
        m_prefilterDepthsRS.AddCBV(0, 0); // constants
        m_prefilterDepthsRS.AddSRV(0, 0); // raw depth
        for (int i = 0; i < XE_GTAO_DEPTH_MIP_LEVELS; i++)
        {
            m_prefilterDepthsRS.AddUAV(i, 0); // working depth mips
        }

        m_prefilterDepthsRS.AddStaticSampler(CD3DX12_STATIC_SAMPLER_DESC(1,
            D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP),
            0, 0);

        m_prefilterDepthsRS.Build(device, true);
        m_prefilterDepthsPSO = CreateComputePipelineState(device,
            L"GTAOPrefilterDepths_CS.cso",
            m_prefilterDepthsRS.Get());

        // --- Working AO term
        auto workingAODesc = CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_R8_UINT,
            windowSize.right,
            windowSize.bottom,
            1, 1
        );
        workingAODesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        m_workingAOTerm.Create(device,
            &workingAODesc,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            nullptr);
        m_workingAOTerm.Get()->SetName(L"GTAO Working AO Term");

        m_workingAOTermSRV = gmm->CreateSRV(device, m_workingAOTerm.Get());
        m_workingAOTermUAV = gmm->CreateUAV(device, m_workingAOTerm.Get());

        m_workingAOTermPong.Create(device,
            &workingAODesc,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            nullptr);
        m_workingAOTermPong.Get()->SetName(L"GTAO Working AO Term Pong");

        m_workingAOTermPongSRV = gmm->CreateSRV(device, m_workingAOTermPong.Get());
        m_workingAOTermPongUAV = gmm->CreateUAV(device, m_workingAOTermPong.Get());

        // --- Edges
        auto edgesDesc = CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_R8_UNORM,
            windowSize.right,
            windowSize.bottom,
            1, 1
        );
        edgesDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        m_edges.Create(device,
            &edgesDesc,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            nullptr);
        m_edges.Get()->SetName(L"GTAO Edges");
        m_edgesUAV = gmm->CreateUAV(device, m_edges.Get());
        m_edgesSRV = gmm->CreateSRV(device, m_edges.Get());

        // --- Main pass RS and PSO
        m_mainPassRS.AddCBV(0, 0); // constants
        m_mainPassRS.AddSRV(0, 0); // depth with mips
        m_mainPassRS.AddSRV(1, 0); // normals
        m_mainPassRS.AddUAV(0, 0); // working AO term
        m_mainPassRS.AddUAV(1, 0); // edges

        m_mainPassRS.AddStaticSampler(CD3DX12_STATIC_SAMPLER_DESC(1,
            D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP),
            0, 0);

        m_mainPassRS.Build(device, true);
        m_mainPassPSO = CreateComputePipelineState(device,
            L"GTAOMainPass_CS.cso",
            m_mainPassRS.Get());

        // --- Denoise pass RS and PSO
        m_denoisePassRS.AddCBV(0, 0); // constants
        m_denoisePassRS.AddSRV(0, 0); // working AO term input
        m_denoisePassRS.AddSRV(1, 0); // edges
        m_denoisePassRS.AddUAV(0, 0); // final AO term

        m_denoisePassRS.AddStaticSampler(CD3DX12_STATIC_SAMPLER_DESC(1,
            D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP),
            0, 0);

        m_denoisePassRS.Build(device, true);
        m_denoisePassPSO = CreateComputePipelineState(device,
            L"GTAODenoisePass_CS.cso",
            m_denoisePassRS.Get());
    }

    void GTAOProcessor::Process(
        ID3D12GraphicsCommandList* cl,
        BarrierResource* depthBuffer,
        const Camera* viewingCamera,
        RECT windowSize)
    {

        XeGTAO::GTAOConstants consts;

        auto proj = viewingCamera->GetProjectionMatrix();

        XeGTAO::GTAOUpdateConstants(consts,
            windowSize.right,
            windowSize.bottom,
            m_gtaoSettings,
            &proj._11,
            true,
            0 // not using TAA
        );

        // TODO: This should probably be done outside, 
        // this class shouldn't need to know if the buffer is multisampled 
        // or not
        depthBuffer->Transition(cl, D3D12_RESOURCE_STATE_RESOLVE_SOURCE);
        m_inputDepthBuffer.Transition(cl, D3D12_RESOURCE_STATE_RESOLVE_DEST);
        cl->ResolveSubresource(m_inputDepthBuffer.Get(), 0,
            depthBuffer->Get(), 0,
            DXGI_FORMAT_R32_FLOAT);


        // Supply the constants to all passes
        // TODO: Batch all the barrier transitions

        // Generate normals pass
        // - pass MSAA resolved depth as an SRV
        // - pass normals output as a UAV
        // - normals should be in R11G11B10_UNORM format
        // - the resource format is DXGI_FORMAT_R32_UINT
        // - see https://github.com/GameTechDev/XeGTAO/blob/master/Source/Rendering/Shaders/vaGTAO.hlsl#L151 for encoding / decoding details
        cl->SetPipelineState(m_generateNormalsPSO.Get());
        m_generateNormalsRS.SetOnCommandList(cl);

        m_inputDepthBuffer.Transition(cl, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        m_normals.Transition(cl, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        m_generateNormalsRS.SetCBV(cl, 0, 0, consts);
        m_generateNormalsRS.SetSRV(cl, 0, 0, m_inputDepthBufferSRV);
        m_generateNormalsRS.SetUAV(cl, 0, 0, m_normalsUAV);

        cl->Dispatch(Math::DivRoundUp(windowSize.right, XE_GTAO_NUMTHREADS_X),
            Math::DivRoundUp(windowSize.bottom, XE_GTAO_NUMTHREADS_Y),
            1);

        // Prefilter depths pass
        // - basically this generates a Hi-Z
        // - pass MSAA resolved depth as an SRV
        // - pass 5 mip levels of the output as UAVs
        cl->SetPipelineState(m_prefilterDepthsPSO.Get());
        m_prefilterDepthsRS.SetOnCommandList(cl);

        m_workingDepths.Transition(cl, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        m_prefilterDepthsRS.SetCBV(cl, 0, 0, consts);
        m_prefilterDepthsRS.SetSRV(cl, 0, 0, m_inputDepthBufferSRV);
        for (int i = 0; i < XE_GTAO_DEPTH_MIP_LEVELS; i++)
        {
            m_prefilterDepthsRS.SetUAV(cl, i, 0, m_workingDepthsUAVs[i]);
        }

        cl->Dispatch(Math::DivRoundUp(windowSize.right, 16),
            Math::DivRoundUp(windowSize.bottom, 16),
            1);

        // Main pass
        // - pass prefiltered depth as an SRV (including all mips)
        // - pass normals from earlier as an SRV. Decode in shader as described above
        // - pass working AO texture as a UAV
        // - AO is R8_UINT
        // - pass edges texture as a UAV
        // - this is R8_UNORM
        // - see https://github.com/GameTechDev/XeGTAO/blob/master/Source/Rendering/Shaders/vaGTAO.hlsl#L118
        cl->SetPipelineState(m_mainPassPSO.Get());
        m_mainPassRS.SetOnCommandList(cl);

        m_workingDepths.Transition(cl, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        m_normals.Transition(cl, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        m_workingAOTerm.Transition(cl, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        m_edges.Transition(cl, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        m_mainPassRS.SetCBV(cl, 0, 0, consts);
        m_mainPassRS.SetSRV(cl, 0, 0, m_workingDepthsSRV);
        m_mainPassRS.SetSRV(cl, 1, 0, m_normalsSRV);
        m_mainPassRS.SetUAV(cl, 0, 0, m_workingAOTermUAV);
        m_mainPassRS.SetUAV(cl, 1, 0, m_edgesUAV);

        cl->Dispatch(Math::DivRoundUp(windowSize.right, XE_GTAO_NUMTHREADS_X),
            Math::DivRoundUp(windowSize.bottom, XE_GTAO_NUMTHREADS_Y),
            1);

        // Denoise passes
        // - pass working AO texture from previous pass as SRV
        // - pass edges from previous pass as an SRV
        // - pass final AO term as a UAV
        // - denoise more than once, ping-pong between two textures 
        // - use loop from here https://github.com/GameTechDev/XeGTAO/blob/master/Source/Rendering/Effects/vaGTAO.cpp#L382
        cl->SetPipelineState(m_denoisePassPSO.Get());
        m_denoisePassRS.SetOnCommandList(cl);

        struct DenoisePassConstants
        {
            XeGTAO::GTAOConstants GTAOConstants;
            bool IsFinalPass = false;
        };

        DenoisePassConstants constants{ consts, false };

        m_edges.Transition(cl, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        const int passCount = std::max(1, m_gtaoSettings.DenoisePasses); // even without denoising we have to run a single last pass to output correct term into the external output texture
        m_readFromPong = false;
        for (int i = 0; i < passCount; i++)
        {
            const bool lastPass = i == passCount - 1;
            constants.IsFinalPass = lastPass;

            m_denoisePassRS.SetCBV(cl, 0, 0, constants);
            m_denoisePassRS.SetSRV(cl, 1, 0, m_edgesSRV);
            if (m_readFromPong)
            {
                m_workingAOTermPong.Transition(cl, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
                m_workingAOTerm.Transition(cl, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                m_denoisePassRS.SetSRV(cl, 0, 0, m_workingAOTermPongSRV);
                m_denoisePassRS.SetUAV(cl, 0, 0, m_workingAOTermUAV);
            }
            else
            {
                m_workingAOTermPong.Transition(cl, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                m_workingAOTerm.Transition(cl, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
                m_denoisePassRS.SetSRV(cl, 0, 0, m_workingAOTermSRV);
                m_denoisePassRS.SetUAV(cl, 0, 0, m_workingAOTermPongUAV);
            }

            cl->Dispatch(
                Math::DivRoundUp(windowSize.right, XE_GTAO_NUMTHREADS_X * 2),
                Math::DivRoundUp(windowSize.bottom, XE_GTAO_NUMTHREADS_Y),
                1
            );

            m_readFromPong = !m_readFromPong;      // ping becomes pong, pong becomes ping.
        }
    }

    GraphicsMemoryManager::DescriptorView GTAOProcessor::GetSRV(ID3D12GraphicsCommandList* cl)
    {
        if (m_readFromPong)
        {
            m_workingAOTermPong.Transition(cl, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
            return m_workingAOTermPongSRV;
        }
        
        m_workingAOTerm.Transition(cl, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        return m_workingAOTermSRV;
    }
}