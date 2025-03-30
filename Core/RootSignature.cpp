#include "pch.h"

#include "Core/RootSignature.h"

namespace Gradient
{
    RootSignature::RootSignature()
    {
        for (auto& space : m_cbvSpaceToSlotToRPIndex)
        {
            for (auto& slot : space)
            {
                slot = UINT_MAX;
            }
        }

        for (auto& space : m_srvSpaceToSlotToRPIndex)
        {
            for (auto& slot : space)
            {
                slot = UINT_MAX;
            }
        }
    }

    ID3D12RootSignature* RootSignature::Get()
    {
        return m_rootSignature.Get();
    }

    void RootSignature::Reset()
    {
        m_rootSignature.Reset();
    }

    void RootSignature::AddCBV(UINT slot, UINT space)
    {
        assert(!m_isBuilt);

        m_descRanges.push_back({
            ParameterTypes::RootCBV,
            slot,
            space
            });
        m_cbvSpaceToSlotToRPIndex[space][slot] = m_descRanges.size() - 1;
    }

    // TODO: Add a method to add an SRV as a root parameter
    void RootSignature::AddSRV(UINT slot, UINT space)
    {
        assert(!m_isBuilt);

        m_descRanges.push_back(
            {
               ParameterTypes::DescriptorTableSRV,
               slot,
               space
            });
        m_srvSpaceToSlotToRPIndex[space][slot] = m_descRanges.size() - 1;
    }

    void RootSignature::AddRootSRV(UINT slot, UINT space)
    {
        assert(!m_isBuilt);

        m_descRanges.push_back(
            {
                ParameterTypes::RootSRV,
                slot,
                space
            });
        m_srvSpaceToSlotToRPIndex[space][slot] = m_descRanges.size() - 1;
    }

    void RootSignature::AddStaticSampler(CD3DX12_STATIC_SAMPLER_DESC samplerDesc,
        UINT slot,
        UINT space)
    {
        assert(!m_isBuilt);

        samplerDesc.ShaderRegister = slot;
        samplerDesc.RegisterSpace = space;

        m_staticSamplers.push_back(samplerDesc);
    }

    void RootSignature::Build(ID3D12Device* device)
    {
        std::vector<CD3DX12_ROOT_PARAMETER1> rootParameters;

        // These descriptor ranges need to be kept around until 
        // the root signature is built
        std::vector<CD3DX12_DESCRIPTOR_RANGE1> descriptorRanges;
        descriptorRanges.reserve(m_descRanges.size());
        
        for (int i = 0; i < m_descRanges.size(); i++)
        {
            CD3DX12_ROOT_PARAMETER1 rp;
            switch (m_descRanges[i].Type)
            {
            case ParameterTypes::RootCBV:
                rp.InitAsConstantBufferView(m_descRanges[i].Slot,
                    m_descRanges[i].Space);
                rootParameters.push_back(rp);
                break;

            case ParameterTypes::DescriptorTableSRV:
                descriptorRanges.push_back({});
                descriptorRanges[descriptorRanges.size() - 1].Init(
                    D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
                    1, m_descRanges[i].Slot, m_descRanges[i].Space);
                rp.InitAsDescriptorTable(1, &descriptorRanges[descriptorRanges.size() - 1]);
                rootParameters.push_back(rp);
                break;

            case ParameterTypes::RootSRV:
                rp.InitAsShaderResourceView(m_descRanges[i].Slot,
                    m_descRanges[i].Space);
                rootParameters.push_back(rp);
                break;

            default:
                throw std::runtime_error("Unsupported descriptor range type!");
                break;
            }
        }

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSig(rootParameters.size(),
            rootParameters.data(),
            m_staticSamplers.size(),
            m_staticSamplers.data());

        rootSig.Desc_1_0.Flags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
        using Microsoft::WRL::ComPtr;

        ComPtr<ID3DBlob> serializedRootSig;
        ComPtr<ID3DBlob> rootSigErrors;

        DX::ThrowIfFailed(D3D12SerializeVersionedRootSignature(&rootSig,
            serializedRootSig.ReleaseAndGetAddressOf(),
            rootSigErrors.ReleaseAndGetAddressOf()));

        DX::ThrowIfFailed(
            device->CreateRootSignature(
                0,
                serializedRootSig->GetBufferPointer(),
                serializedRootSig->GetBufferSize(),
                IID_PPV_ARGS(m_rootSignature.ReleaseAndGetAddressOf())
            ));

        m_isBuilt = true;
    }

    void RootSignature::SetSRV(ID3D12GraphicsCommandList* cl,
        UINT slot,
        UINT space,
        GraphicsMemoryManager::DescriptorView index)
    {
        assert(m_isBuilt);
        if (!index) return;

        auto rpIndex = m_srvSpaceToSlotToRPIndex[space][slot];
        assert(rpIndex != UINT32_MAX);
        cl->SetGraphicsRootDescriptorTable(rpIndex,
            index->GetGPUHandle());
    }

    void RootSignature::SetOnCommandList(ID3D12GraphicsCommandList* cl)
    {
        assert(m_isBuilt);
        cl->SetGraphicsRootSignature(m_rootSignature.Get());
    }
}