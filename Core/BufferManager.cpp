#include "pch.h"

#include "Core/BufferManager.h"

#include <directxtk12/BufferHelpers.h>
#include <directxtk12/ResourceUploadBatch.h>

namespace Gradient
{
    std::unique_ptr<BufferManager> BufferManager::s_instance;

    void BufferManager::Initialize()
    {
        s_instance = std::make_unique<BufferManager>();
    }

    void BufferManager::Shutdown()
    {
        s_instance.reset();
    }

    BufferManager* BufferManager::Get()
    {
        return s_instance.get();
    }

    BufferManager::InstanceBufferHandle BufferManager::CreateInstanceBuffer(
        ID3D12Device* device,
        ID3D12CommandQueue* cq,
        const std::vector<InstanceData>& instanceData)
    {
        std::vector<InstanceData> transposedInstanceData;

        for (const auto& instance : instanceData)
        {
            transposedInstanceData.push_back({
                DirectX::XMMatrixTranspose(instance.World),
                instance.TexcoordURange,
                instance.TexcoordVRange
                });
        }

        auto handle = m_instanceBuffers.Allocate({
            BarrierResource(),
            static_cast<uint32_t>(transposedInstanceData.size())
            });

        auto entry = m_instanceBuffers.Get(handle);

        DirectX::ResourceUploadBatch uploadBatch(device);

        uploadBatch.Begin();

        // TODO: set flag to allow access as a UAV?
        DX::ThrowIfFailed(
            DirectX::CreateStaticBuffer(device,
                uploadBatch,
                transposedInstanceData.data(),
                transposedInstanceData.size(),
                D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
                entry->Resource.ReleaseAndGetAddressOf()));
        entry->Resource.SetState(D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);

        auto uploadFinished = uploadBatch.End(cq);
        uploadFinished.wait();

        return handle;
    }

    BufferManager::InstanceBufferEntry* BufferManager::GetInstanceBuffer(InstanceBufferHandle handle)
    {
        return m_instanceBuffers.Get(handle);
    }
}