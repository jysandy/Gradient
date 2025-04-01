#pragma once

#include "pch.h"

#include <entt/entt.hpp>

namespace Gradient::ECS::Components
{
    struct RelationshipComponent
    {
        entt::entity Parent;
    };
}