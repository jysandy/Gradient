#include "pch.h"

#include "Core/Rendering/Primitive.h"

namespace Gradient::Rendering
{
    Primitive::Primitive() {}

    Primitive::Primitive(std::unique_ptr<DirectX::GeometricPrimitive>&& p)
    {
        m_primitive = std::move(p);
    }

    void Primitive::Draw(Effects::IEntityEffect* effect, std::function<void()> setCustomState)
    {
        m_primitive->Draw(effect,
            effect->GetInputLayout(),
            false,
            false,
            setCustomState);
    }

    std::unique_ptr<Primitive> Primitive::FromGeometricPrimitive(std::unique_ptr<DirectX::GeometricPrimitive>&& p)
    {
        return std::make_unique<Primitive>(std::move(p));
    }
}