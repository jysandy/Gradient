#pragma once

#include "pch.h"
#include <array>
#include "Core/GraphicsMemoryManager.h"

namespace Gradient
{
    class RootSignature
    {
    public:
        RootSignature();

        void AddCBV(UINT slot, UINT space);
        void AddSRV(UINT slot, UINT space);
        void AddStaticSampler(CD3DX12_STATIC_SAMPLER_DESC samplerDesc,
            UINT slot,
            UINT space);

        void Build(ID3D12Device* device);

        template <typename T>
        void SetCBV(ID3D12GraphicsCommandList* cl, 
            UINT slot, 
            UINT space,
            const T& data);
        
        void SetSRV(ID3D12GraphicsCommandList* cl,
            UINT slot,
            UINT space, 
            GraphicsMemoryManager::DescriptorIndex index);

        void SetOnCommandList(ID3D12GraphicsCommandList* cl);

    private:
        bool m_isBuilt = false;

        std::vector<CD3DX12_DESCRIPTOR_RANGE1> m_descRanges;
        std::vector< CD3DX12_STATIC_SAMPLER_DESC> m_staticSamplers;
        std::array<std::array<UINT, 128>, 6> m_spaceToSlotToRPIndex;

        Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
    };

    template <typename T>
    void RootSignature::SetCBV(ID3D12GraphicsCommandList* cl,
        UINT slot,
        UINT space,
        const T& data)
    {
        assert(m_isBuilt);

        auto rpIndex = m_spaceToSlotToRPIndex[space][slot];

        assert(rpIndex != UINT32_MAX);

        auto gmm = GraphicsMemoryManager::Get();
        auto cbv = gmm->AllocateConstant(data);
        cl->SetGraphicsRootConstantBufferView(rpIndex, cbv.GpuAddress());
    }
}