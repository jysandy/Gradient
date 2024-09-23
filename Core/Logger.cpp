#include "pch.h"
#include "Core/Logger.h"
#include "spdlog/sinks/msvc_sink.h"

namespace Gradient
{
    std::shared_ptr<spdlog::logger> Logger::s_logger;

    std::shared_ptr<spdlog::logger> Logger::Get()
    {
        return s_logger;
    }

    void Logger::Initialize()
    {
        auto sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
        s_logger = std::make_shared<spdlog::logger>("GradientLogger", sink);
    }

    void Logger::Destroy()
    {
        s_logger.reset();
    }
}