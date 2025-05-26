#pragma once

#include "pch.h"
#include <array>
#include "Core/GraphicsMemoryManager.h"
#include "Core/BufferManager.h"

namespace Gradient
{
    class RootSignature
    {
    public:
        RootSignature();

        void AddCBV(UINT slot, UINT space);
        void AddSRV(UINT slot, UINT space);
        void AddUAV(UINT slot, UINT space);
        void AddRootSRV(UINT slot, UINT space);
        void AddStaticSampler(CD3DX12_STATIC_SAMPLER_DESC samplerDesc,
            UINT slot,
            UINT space);

        void Build(ID3D12Device* device, bool compute = false);
        ID3D12RootSignature* Get();
        void Reset();

        template <typename T>
        void SetCBV(ID3D12GraphicsCommandList* cl, 
            UINT slot, 
            UINT space,
            const T& data);

        void SetStructuredBufferSRV(ID3D12GraphicsCommandList* cl,
            UINT slot,
            UINT space,
            BufferManager::InstanceBufferHandle handle);
        
        void SetSRV(ID3D12GraphicsCommandList* cl,
            UINT slot,
            UINT space, 
            GraphicsMemoryManager::DescriptorView index);

        void SetUAV(ID3D12GraphicsCommandList* cl,
            UINT slot,
            UINT space,
            GraphicsMemoryManager::DescriptorView index);

        void SetOnCommandList(ID3D12GraphicsCommandList* cl);

    private:
        bool m_isBuilt = false;
        bool m_isCompute = false;

        enum class ParameterTypes
        {
            RootCBV,
            RootSRV,
            DescriptorTableSRV,
            DescriptorTableUAV
        };

        struct ParameterDesc
        {
            ParameterTypes Type;
            UINT Slot;
            UINT Space;
        };

        std::vector<ParameterDesc> m_descRanges;
        std::vector< CD3DX12_STATIC_SAMPLER_DESC> m_staticSamplers;
        std::array<std::array<UINT, 64>, 6> m_cbvSpaceToSlotToRPIndex; // 'b' resources
        std::array<std::array<UINT, 64>, 6> m_srvSpaceToSlotToRPIndex; // 't' resources
        std::array<std::array<UINT, 64>, 6> m_uavSpaceToSlotToRPIndex; // 'u' resources


        Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
    };

    template <typename T>
    void RootSignature::SetCBV(ID3D12GraphicsCommandList* cl,
        UINT slot,
        UINT space,
        const T& data)
    {
        assert(m_isBuilt);

        auto rpIndex = m_cbvSpaceToSlotToRPIndex[space][slot];

        assert(rpIndex != UINT32_MAX);

        auto gmm = GraphicsMemoryManager::Get();
        auto cbvAddress = gmm->AllocateConstant(data);

        if (m_isCompute)
            cl->SetComputeRootConstantBufferView(rpIndex, cbvAddress);
        else
            cl->SetGraphicsRootConstantBufferView(rpIndex, cbvAddress);
    }
}