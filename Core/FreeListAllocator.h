#pragma once

#include "pch.h"
#include <optional>

namespace Gradient
{
    template <typename T>
    class FreeListAllocator
    {
    public:
        using Handle = std::size_t;

        Handle Allocate(const T& in);
        Handle Allocate(T&& in);
        void Remove(Handle handle);

        T* Get(Handle handle);
                

    private:
        std::vector<std::optional<T>> m_elements;
        std::vector<Handle> m_freeHandles;

    };

    template <typename T>
    FreeListAllocator<T>::Handle 
        FreeListAllocator<T>::Allocate(T&& in)
    {
        if (!m_freeHandles.empty())
        {
            Handle handle = m_freeHandles.back();
            m_freeHandles.pop_back();
            m_elements[handle] = in;

            return handle;
        }

        m_elements.emplace_back(in);
        return m_elements.size() - 1;
    }

    template <typename T>
    FreeListAllocator<T>::Handle
        FreeListAllocator<T>::Allocate(const T& in)
    {
        if (!m_freeHandles.empty())
        {
            Handle handle = m_freeHandles.back();
            m_freeHandles.pop_back();
            assert(!m_elements[handle]);
            m_elements[handle] = in;

            return handle;
        }

        m_elements.emplace_back(in);
        return m_elements.size() - 1;
    }

    template <typename T>
    void FreeListAllocator<T>::Remove(FreeListAllocator<T>::Handle handle)
    {
        if (handle < m_elements.size())
        {
            m_freeHandles.push_back(handle);
            m_elements[handle].reset();
        }
    }

    template <typename T>
    T* FreeListAllocator<T>::Get(FreeListAllocator<T>::Handle handle)
    {
        if (handle < m_elements.size())
        {
            return &m_elements[handle].value();
        }

        return nullptr;
    }
}