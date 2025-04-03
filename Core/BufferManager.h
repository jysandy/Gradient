#pragma once

#include "pch.h"

#include "Core/BarrierResource.h"

#include <optional>

namespace Gradient
{
    class BufferManager
    {
    public:
        using InstanceBufferHandle = std::uint64_t;

        struct __declspec(align(16)) InstanceData
        {
            DirectX::XMMATRIX World;
            DirectX::XMFLOAT2 TexcoordURange;
            DirectX::XMFLOAT2 TexcoordVRange;
        };

        struct InstanceBufferEntry
        {
            std::shared_ptr<BarrierResource> Resource;
            uint32_t InstanceCount;
        };

        static void Initialize();
        static void Shutdown();
        static BufferManager* Get();

        InstanceBufferHandle CreateInstanceBuffer(ID3D12Device* device,
            ID3D12CommandQueue* cq,
            const std::vector<InstanceData>& instanceData);
        std::optional<InstanceBufferEntry> GetInstanceBuffer(InstanceBufferHandle handle);


    private:
        static std::unique_ptr<BufferManager> s_instance;

        InstanceBufferHandle m_nextIBH = 0;
        InstanceBufferHandle AllocateInstanceBufferHandle();

        // TODO: Track the size of the instance buffer here.
        // TODO: Change this to a vector with a free list
        std::unordered_map<InstanceBufferHandle, InstanceBufferEntry> m_instanceBuffers;

    };
}