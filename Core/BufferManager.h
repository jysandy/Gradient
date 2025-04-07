#pragma once

#include "pch.h"

#include "Core/BarrierResource.h"
#include "Core/FreeListAllocator.h"

#include <optional>

namespace Gradient
{
    class BufferManager
    {
    public:
        struct __declspec(align(16)) InstanceData
        {
            DirectX::XMMATRIX World;
            DirectX::XMFLOAT2 TexcoordURange;
            DirectX::XMFLOAT2 TexcoordVRange;
        };

        struct InstanceBufferEntry
        {
            BarrierResource Resource;
            uint32_t InstanceCount;
        };

        using InstanceBufferList = FreeListAllocator<InstanceBufferEntry>;
        using InstanceBufferHandle = InstanceBufferList::Handle;

        static void Initialize();
        static void Shutdown();
        static BufferManager* Get();

        InstanceBufferHandle CreateInstanceBuffer(ID3D12Device* device,
            ID3D12CommandQueue* cq,
            const std::vector<InstanceData>& instanceData);
        InstanceBufferEntry* GetInstanceBuffer(InstanceBufferHandle handle);


    private:
        static std::unique_ptr<BufferManager> s_instance;

        InstanceBufferList m_instanceBuffers;
    };
}