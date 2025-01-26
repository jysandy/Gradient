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