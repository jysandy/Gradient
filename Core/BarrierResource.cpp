#include "pch.h"

#include "Core/BarrierResource.h"
#include <directxtk12/DirectXHelpers.h>

namespace Gradient
{
    BarrierResource::BarrierResource(D3D12_RESOURCE_STATES initialState)
        : m_resourceState(initialState)
    {

    }

    ID3D12Resource* BarrierResource::Get()
    {
        return m_resource.Get();
    }

    ID3D12Resource** BarrierResource::ReleaseAndGetAddressOf()
    {
        return m_resource.ReleaseAndGetAddressOf();
    }

    void BarrierResource::Create(
        ID3D12Device* device,
        const D3D12_RESOURCE_DESC* desc,
        D3D12_RESOURCE_STATES       initialResourceState,
        const D3D12_CLEAR_VALUE* optimizedClearValue
    )
    {
        auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

        DX::ThrowIfFailed(
            device->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                desc,
                initialResourceState,
                optimizedClearValue,
                IID_PPV_ARGS(m_resource.ReleaseAndGetAddressOf())));
        SetState(initialResourceState);
    }

    void BarrierResource::SetState(D3D12_RESOURCE_STATES newState)
    {
        m_resourceState = newState;
    }

    void BarrierResource::Transition(ID3D12GraphicsCommandList* cl,
        D3D12_RESOURCE_STATES newState)
    {
        DirectX::TransitionResource(cl,
            m_resource.Get(),
            m_resourceState,
            newState);
        m_resourceState = newState; // Not updated on the GPU yet!
    }
}