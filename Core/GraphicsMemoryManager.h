#pragma once

#include "pch.h"

#include <set>
#include <map>
#include <directxtk12/DescriptorHeap.h>
#include <directxtk12/DirectXHelpers.h>

namespace Gradient
{
    // Manages resources and descriptors. Not currently thread-safe.
    class GraphicsMemoryManager
    {
    public:
        // TODO: Make these things free themselves.
        // The descriptor heap runs out of space whenever the 
        // window is resized because the old descriptors are 
        // never reclaimed.
        using DescriptorIndex = DirectX::DescriptorPile::IndexType;

        static void Initialize(ID3D12Device* device);
        static void Shutdown();

        static GraphicsMemoryManager* Get();

        template <typename T>
        inline DirectX::GraphicsResource AllocateConstant(const T& data);

        void Commit(ID3D12CommandQueue* cq);


        // SRV

        DescriptorIndex AllocateSrv();
        void FreeSrv(DescriptorIndex index);
        // Used by ImGui
        void FreeSrvByCpuHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle);

        DescriptorIndex CreateSRV(
            ID3D12Device* device,
            ID3D12Resource* resource,
            bool isCubeMap = false
        );

        DescriptorIndex CreateSRV(
            ID3D12Device* device,
            ID3D12Resource* resource,
            D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc
        );

        D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCpuHandle(DescriptorIndex index);
        D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGpuHandle(DescriptorIndex index);

        ID3D12DescriptorHeap* GetSrvDescriptorHeap();


        // RTV

        DescriptorIndex AllocateRTV();

        DescriptorIndex CreateRTV(
            ID3D12Device* device,
            ID3D12Resource* resource
        );
                   
        DescriptorIndex CreateRTV(
            ID3D12Device* device,
            D3D12_RENDER_TARGET_VIEW_DESC desc,
            ID3D12Resource* resource
        );

        D3D12_CPU_DESCRIPTOR_HANDLE GetRTVCpuHandle(DescriptorIndex index);


        // DSV

        DescriptorIndex AllocateDSV();

        DescriptorIndex CreateDSV(
            ID3D12Device* device,
            ID3D12Resource* resource,
            D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc
        );

        D3D12_CPU_DESCRIPTOR_HANDLE GetDSVCpuHandle(DescriptorIndex index);

    private:
        GraphicsMemoryManager(ID3D12Device* device);
        void TrackCpuDescriptorHandle(DescriptorIndex index);
        void UntrackCpuDescriptorHandle(DescriptorIndex index);

        static std::unique_ptr<GraphicsMemoryManager> s_instance;

        std::unique_ptr<DirectX::GraphicsMemory> m_graphicsMemory;
        std::unique_ptr<DirectX::DescriptorPile> m_srvDescriptors;
        std::unique_ptr<DirectX::DescriptorPile> m_rtvDescriptors;
        std::unique_ptr<DirectX::DescriptorPile> m_dsvDescriptors;

        std::set<DescriptorIndex> m_freeSrvIndices;

        struct DescriptorHandleHash
        {
            std::size_t operator()(const D3D12_CPU_DESCRIPTOR_HANDLE& h) const noexcept
            {
                return std::hash<SIZE_T>{}(h.ptr);
            }
        };

        struct DescriptorHandleEqual
        {
            constexpr bool operator()(const D3D12_CPU_DESCRIPTOR_HANDLE& l,
                const D3D12_CPU_DESCRIPTOR_HANDLE& r) const
            {
                return l.ptr == r.ptr;
            }
        };

        std::unordered_map<D3D12_CPU_DESCRIPTOR_HANDLE, 
            DescriptorIndex, 
            DescriptorHandleHash,
            DescriptorHandleEqual> 
            m_cpuHandleToSrvIndex;
    };

    template <typename T>
    inline DirectX::GraphicsResource GraphicsMemoryManager::AllocateConstant(const T& data)
    {
        return m_graphicsMemory->AllocateConstant(data);
    }
}