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

	consteval auto MakeOffset(
		[[maybe_unused]] std::uint64_t a_idSE,
		[[maybe_unused]] std::uint64_t a_addrVR)
	{
#ifndef SKYRIMVR
		return REL::ID(a_idSE);
#else
		return REL::Offset(a_addrVR);
#endif
	}
}

#ifndef SKYRIMVR
#define IF_SKYRIMSE(a_ifSE, a_ifVR) a_ifSE
#else
#define IF_SKYRIMSE(a_ifSE, a_ifVR) a_ifVR
#endif

#define DLLEXPORT __declspec(dllexport)

#include "Plugin.h"
