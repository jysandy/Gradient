#pragma once

#include <spdlog/spdlog.h>
#include <memory>

namespace Gradient
{
    class Logger
    {
    private:
        static std::shared_ptr<spdlog::logger> s_logger;

    public:
        static std::shared_ptr<spdlog::logger> Get();
        static void Initialize();
        static void Destroy();
    };
}