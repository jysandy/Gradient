#pragma once

#include "pch.h"

namespace Gradient
{
    class BarrierResource
    {
    public:
        BarrierResource() = default;
        BarrierResource(D3D12_RESOURCE_STATES initialState);

        ID3D12Resource* Get();
        ID3D12Resource** ReleaseAndGetAddressOf();

        D3D12_GPU_VIRTUAL_ADDRESS GetGpuAddress() const;

        void Create(
            ID3D12Device* device,
            const D3D12_RESOURCE_DESC* pDesc,
            D3D12_RESOURCE_STATES       InitialResourceState,
            const D3D12_CLEAR_VALUE* pOptimizedClearValue
        );

        void SetState(D3D12_RESOURCE_STATES newState);
        void Transition(ID3D12GraphicsCommandList* cl,
            D3D12_RESOURCE_STATES newState);

    private:
        Microsoft::WRL::ComPtr<ID3D12Resource> m_resource;

        // This state is not guaranteed to be in sync with the GPU!
        // Resource state transitions are recorded on a command list 
        // and executed on the GPU later, so the CPU resource state 
        // is typically ahead of the GPU.
        // TODO: Consider tracking the CPU and GPU states separately and 
        // syncing them at the start of every frame.
        D3D12_RESOURCE_STATES m_resourceState;
    };
}