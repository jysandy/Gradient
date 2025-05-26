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
            64);
        m_dsvDescriptors = std::make_unique<DirectX::DescriptorPile>(device,
            D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
            D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
            64);
    }

    void GraphicsMemoryManager::Commit(ID3D12CommandQueue* cq)
    {
        m_frameGraphicsResources.clear();
        m_graphicsMemory->Commit(cq);
    }

    GraphicsMemoryManager::DescriptorIndex GraphicsMemoryManager::AllocateSrvOrUav()
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

    void GraphicsMemoryManager::FreeSrvOrUav(GraphicsMemoryManager::DescriptorIndex index)
    {
        UntrackCpuDescriptorHandle(index);
        m_freeSrvIndices.insert(index);
    }

    void GraphicsMemoryManager::FreeSrvByCpuHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle)
    {
        if (auto it = m_cpuHandleToSrvIndex.find(cpuHandle); it != m_cpuHandleToSrvIndex.end())
        {
            FreeSrvOrUav(it->second);
        }
    }

    GraphicsMemoryManager::DescriptorView GraphicsMemoryManager::CreateSRV(
        ID3D12Device* device,
        ID3D12Resource* resource,
        bool isCubeMap
    )
    {
        auto index = AllocateSrvOrUav();

        DirectX::CreateShaderResourceView(device,
            resource,
            GetSRVOrUAVCpuHandle(index),
            isCubeMap);

        return std::make_shared<DescriptorIndexContainer>(index, DescriptorIndexType::SRVorUAV);
    }

    GraphicsMemoryManager::DescriptorView GraphicsMemoryManager::CreateUAV(
        ID3D12Device* device,
        ID3D12Resource* resource,
        uint32_t mipLevel
    )
    {
        auto index = AllocateSrvOrUav();

        DirectX::CreateUnorderedAccessView(device, resource, GetSRVOrUAVCpuHandle(index), mipLevel);

        return std::make_shared<DescriptorIndexContainer>(index, DescriptorIndexType::SRVorUAV);
    }

    GraphicsMemoryManager::DescriptorView GraphicsMemoryManager::CreateSRV(
        ID3D12Device* device,
        ID3D12Resource* resource,
        D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc
    )
    {
        auto index = AllocateSrvOrUav();

        device->CreateShaderResourceView(resource,
            srvDesc, GetSRVOrUAVCpuHandle(index));

        return std::make_shared<DescriptorIndexContainer>(index, DescriptorIndexType::SRVorUAV);
    }

    GraphicsMemoryManager::DescriptorIndex GraphicsMemoryManager::AllocateRTV()
    {
        if (!m_freeRTVIndices.empty())
        {
            auto index_it = m_freeRTVIndices.begin();
            auto index = *index_it;
            m_freeRTVIndices.erase(index_it);
            return index;
        }

        return m_rtvDescriptors->Allocate();
    }

    void GraphicsMemoryManager::FreeRTV(DescriptorIndex index)
    {
        m_freeRTVIndices.insert(index);
    }

    GraphicsMemoryManager::DescriptorView GraphicsMemoryManager::CreateRTV(
        ID3D12Device* device,
        ID3D12Resource* resource
    )
    {
        auto index = AllocateRTV();

        DirectX::CreateRenderTargetView(device,
            resource,
            GetRTVCpuHandle(index));

        return std::make_shared<DescriptorIndexContainer>(index, DescriptorIndexType::RTV);
    }

    GraphicsMemoryManager::DescriptorView GraphicsMemoryManager::CreateRTV(
        ID3D12Device* device,
        D3D12_RENDER_TARGET_VIEW_DESC desc,
        ID3D12Resource* resource
    )
    {
        auto index = AllocateRTV();

        device->CreateRenderTargetView(
            resource,
            &desc,
            GetRTVCpuHandle(index)
        );

        return std::make_shared<DescriptorIndexContainer>(index, DescriptorIndexType::RTV);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE GraphicsMemoryManager::GetRTVCpuHandle(DescriptorIndex index)
    {
        return m_rtvDescriptors->GetCpuHandle(index);
    }

    GraphicsMemoryManager::DescriptorIndex GraphicsMemoryManager::AllocateDSV()
    {
        if (!m_freeDSVIndices.empty())
        {
            auto index_it = m_freeDSVIndices.begin();
            auto index = *index_it;
            m_freeDSVIndices.erase(index_it);
            return index;
        }

        return m_dsvDescriptors->Allocate();
    }

    void GraphicsMemoryManager::FreeDSV(DescriptorIndex index)
    {
        m_freeDSVIndices.insert(index);
    }

    GraphicsMemoryManager::DescriptorView GraphicsMemoryManager::CreateDSV(
        ID3D12Device* device,
        ID3D12Resource* resource,
        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc
    )
    {
        auto index = AllocateDSV();

        device->CreateDepthStencilView(resource,
            &dsvDesc,
            GetDSVCpuHandle(index));

        return std::make_shared<DescriptorIndexContainer>(index, DescriptorIndexType::DSV);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE GraphicsMemoryManager::GetDSVCpuHandle(DescriptorIndex index)
    {
        return m_dsvDescriptors->GetCpuHandle(index);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE GraphicsMemoryManager::GetSRVOrUAVCpuHandle(DescriptorIndex index)
    {
        return m_srvDescriptors->GetCpuHandle(index);
    }

    D3D12_GPU_DESCRIPTOR_HANDLE GraphicsMemoryManager::GetSRVOrUAVGpuHandle(DescriptorIndex index)
    {
        return m_srvDescriptors->GetGpuHandle(index);
    }

    ID3D12DescriptorHeap* GraphicsMemoryManager::GetSrvUavDescriptorHeap()
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

    GraphicsMemoryManager::DescriptorIndexContainer::DescriptorIndexContainer(DescriptorIndex index, DescriptorIndexType indexType)
        : m_index(index), m_indexType(indexType)
    {
    }

    GraphicsMemoryManager::DescriptorIndexContainer::~DescriptorIndexContainer()
    {
        auto gmm = GraphicsMemoryManager::Get();
        
        if (gmm == nullptr) return; // the memory has already been freed

        switch (m_indexType)
        {
        case DescriptorIndexType::SRVorUAV:
            gmm->FreeSrvOrUav(m_index);
            break; 
        case DescriptorIndexType::RTV:
            gmm->FreeRTV(m_index);
            break;
        case DescriptorIndexType::DSV:
            gmm->FreeDSV(m_index);
            break;
        }
    }

    D3D12_CPU_DESCRIPTOR_HANDLE GraphicsMemoryManager::DescriptorIndexContainer::GetCPUHandle()
    {
        auto gmm = GraphicsMemoryManager::Get();

        switch (m_indexType)
        {
        case DescriptorIndexType::SRVorUAV:
            return gmm->GetSRVOrUAVCpuHandle(m_index);
            break;
        case DescriptorIndexType::RTV:
            return gmm->GetRTVCpuHandle(m_index);
            break;
        case DescriptorIndexType::DSV:
            return gmm->GetDSVCpuHandle(m_index);
            break;
        }
    }

    D3D12_GPU_DESCRIPTOR_HANDLE GraphicsMemoryManager::DescriptorIndexContainer::GetGPUHandle()
    {
        assert(m_indexType == DescriptorIndexType::SRVorUAV);
        auto gmm = GraphicsMemoryManager::Get();

        return gmm->GetSRVOrUAVGpuHandle(m_index);
    }
}