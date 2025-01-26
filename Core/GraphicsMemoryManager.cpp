#include "pch.h"

#include "Core/GraphicsMemoryManager.h"

namespace Gradient
{
    std::unique_ptr<GraphicsMemoryManager> GraphicsMemoryManager::s_instance;

    GraphicsMemoryManager::GraphicsMemoryManager(ID3D12Device* device)
    {
        m_graphicsMemory = std::make_unique<DirectX::GraphicsMemory>(device);

        m_srvDescriptors = std::make_unique<DirectX::DescriptorPile>(device,
            256);
        m_rtvDescriptors = std::make_unique<DirectX::DescriptorPile>(device,
            D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
            D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
            32);
        m_rtvDescriptors = std::make_unique<DirectX::DescriptorPile>(device,
            D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
            D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
            32);
    }

    void GraphicsMemoryManager::Commit(ID3D12CommandQueue* cq)
    {
        m_graphicsMemory->Commit(cq);
    }

    GraphicsMemoryManager::DescriptorIndex GraphicsMemoryManager::AllocateSrv()
    {
        if (!m_freeSrvIndices.empty())
        {
            auto index_it = m_freeSrvIndices.begin();
            auto index = *index_it;
            m_freeSrvIndices.erase(index_it);
            TrackCpuDescriptorHandle(index);
            return index;
        }

        auto index = m_srvDescriptors->Allocate();
        TrackCpuDescriptorHandle(index);
        return index;
    }

    void GraphicsMemoryManager::TrackCpuDescriptorHandle(DescriptorIndex index)
    {
        auto cpuHandle = m_srvDescriptors->GetCpuHandle(index);
        m_cpuHandleToSrvIndex.insert({ cpuHandle, index });
    }

    void GraphicsMemoryManager::UntrackCpuDescriptorHandle(DescriptorIndex index)
    {
        auto cpuHandle = m_srvDescriptors->GetCpuHandle(index);
        m_cpuHandleToSrvIndex.erase(cpuHandle);
    }

    void GraphicsMemoryManager::FreeSrv(GraphicsMemoryManager::DescriptorIndex index)
    {
        UntrackCpuDescriptorHandle(index);
        m_freeSrvIndices.insert(index);
    }

    void GraphicsMemoryManager::FreeSrvByCpuHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle)
    {
        if (auto it = m_cpuHandleToSrvIndex.find(cpuHandle); it != m_cpuHandleToSrvIndex.end())
        {
            FreeSrv(it->second);
        }
    }

    GraphicsMemoryManager::DescriptorIndex GraphicsMemoryManager::CreateSRV(
        ID3D12Device* device,
        ID3D12Resource* resource,
        bool isCubeMap
    )
    {
        auto index = AllocateSrv();

        DirectX::CreateShaderResourceView(device,
            resource,
            GetSRVCpuHandle(index),
            isCubeMap);

        return index;
    }

    GraphicsMemoryManager::DescriptorIndex GraphicsMemoryManager::CreateSRV(
        ID3D12Device* device,
        ID3D12Resource* resource,
        D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc
    )
    {
        auto index = AllocateSrv();

        device->CreateShaderResourceView(resource,
            srvDesc, GetSRVCpuHandle(index));

        return index;
    }

    GraphicsMemoryManager::DescriptorIndex GraphicsMemoryManager::AllocateRTV()
    {
        return m_rtvDescriptors->Allocate();
    }

    GraphicsMemoryManager::DescriptorIndex GraphicsMemoryManager::CreateRTV(
        ID3D12Device* device,
        ID3D12Resource* resource
    )
    {
        auto index = AllocateRTV();

        DirectX::CreateRenderTargetView(device,
            resource,
            GetRTVCpuHandle(index));

        return index;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE GraphicsMemoryManager::GetRTVCpuHandle(DescriptorIndex index)
    {
        return m_rtvDescriptors->GetCpuHandle(index);
    }

    GraphicsMemoryManager::DescriptorIndex GraphicsMemoryManager::AllocateDSV()
    {
        return m_dsvDescriptors->Allocate();
    }

    GraphicsMemoryManager::DescriptorIndex GraphicsMemoryManager::CreateDSV(
        ID3D12Device* device,
        ID3D12Resource* resource,
        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc
    )
    {
        auto index = AllocateDSV();

        device->CreateDepthStencilView(resource,
            &dsvDesc,
            GetDSVCpuHandle(index));

        return index;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE GraphicsMemoryManager::GetDSVCpuHandle(DescriptorIndex index)
    {
        return m_dsvDescriptors->GetCpuHandle(index);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE GraphicsMemoryManager::GetSRVCpuHandle(DescriptorIndex index)
    {
        return m_srvDescriptors->GetCpuHandle(index);
    }

    D3D12_GPU_DESCRIPTOR_HANDLE GraphicsMemoryManager::GetSRVGpuHandle(DescriptorIndex index)
    {
        return m_srvDescriptors->GetGpuHandle(index);
    }

    ID3D12DescriptorHeap* GraphicsMemoryManager::GetSrvDescriptorHeap()
    {
        return m_srvDescriptors->Heap();
    }

    void GraphicsMemoryManager::Initialize(ID3D12Device* device)
    {
        auto instance = new GraphicsMemoryManager(device);
        s_instance = std::unique_ptr<GraphicsMemoryManager>(instance);
    }

    void GraphicsMemoryManager::Shutdown()
    {
        s_instance.reset();
    }

    GraphicsMemoryManager* GraphicsMemoryManager::Get()
    {
        return s_instance.get();
    }
}