#pragma once

#include "pch.h"
#include <memory>
#include "Core/Rendering/IDrawable.h"

#include <directxtk/GeometricPrimitive.h>

namespace Gradient::Rendering
{
    class Primitive : public IDrawable
    {
    public:
        Primitive();
        Primitive(std::unique_ptr<DirectX::GeometricPrimitive>&& p);
        virtual ~Primitive() = default;

        virtual void Draw(Effects::IEntityEffect* effect, std::function<void()> setCustomState = nullptr) override;

        static std::unique_ptr<Primitive> FromGeometricPrimitive(std::unique_ptr<DirectX::GeometricPrimitive>&& p);

    private:
        std::unique_ptr<DirectX::GeometricPrimitive> m_primitive;
    };
}