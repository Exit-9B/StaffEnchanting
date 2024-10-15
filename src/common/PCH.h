#pragma once

#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>

#include <ranges>

#ifdef NDEBUG
#include <spdlog/sinks/basic_file_sink.h>
#else
#include <spdlog/sinks/msvc_sink.h>
#endif

using namespace std::literals;
using namespace RE::literals;

namespace logger = SKSE::log;

namespace util
{
	using SKSE::stl::report_and_fail;
	using SKSE::stl::to_underlying;
}

#define DLLEXPORT __declspec(dllexport)

#include "Plugin.h"
