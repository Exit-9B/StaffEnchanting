#include "Hooks/BaseCharge.h"
#include "Hooks/Create.h"
#include "Hooks/Workbench.h"
#include "Settings/JSONSettings.h"
#include "UI/StaffCraftingMenu.h"
#include "common/Forms.h"

namespace
{
	void InitializeLog()
	{
#ifndef NDEBUG
		auto sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
#else
		auto path = logger::log_directory();
		if (!path) {
			util::report_and_fail("Failed to find standard logging directory"sv);
		}

		*path /= fmt::format("{}.log"sv, Plugin::NAME);
		auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
#endif

#ifndef NDEBUG
		const auto level = spdlog::level::trace;
#else
		const auto level = spdlog::level::info;
#endif

		auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));
		log->set_level(level);
		log->flush_on(level);

		spdlog::set_default_logger(std::move(log));
		spdlog::set_pattern("%s(%#): [%^%l%$] %v"s);
	}
}

extern "C" DLLEXPORT constinit auto SKSEPlugin_Version = []()
{
	SKSE::PluginVersionData v{};

	v.PluginVersion(Plugin::VERSION);
	v.PluginName(Plugin::NAME);
	v.AuthorName("Parapets"sv);

	v.UsesAddressLibrary(true);

	return v;
}();

extern "C" DLLEXPORT bool SKSEAPI
SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
{
	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->name = Plugin::NAME.data();
	a_info->version = Plugin::VERSION[0];

	if (a_skse->IsEditor()) {
		return false;
	}

	const auto ver = a_skse->RuntimeVersion();
	if (ver < SKSE::RUNTIME_1_6_1130) {
		return false;
	}

	return true;
}

static void MessageEventCallback(SKSE::MessagingInterface::Message* a_msg)
{
	switch (a_msg->type) {
	case SKSE::MessagingInterface::kDataLoaded:
	{
		UI::StaffCraftingMenu::Register();

		if (const auto
				HelpStaffEnchantingLong = Forms::StaffEnchanting::HelpStaffEnchantingLong()) {

			if (const auto HelpManualPC = RE::TESForm::LookupByID<RE::BGSListForm>(0x163)) {
				HelpManualPC->AddForm(HelpStaffEnchantingLong);
			}

			if (const auto HelpManualXBox = RE::TESForm::LookupByID<RE::BGSListForm>(0x165)) {
				HelpManualXBox->AddForm(HelpStaffEnchantingLong);
			}
		}
		JSONSettings::SettingsHolder::GetSingleton()->Read();
	} break;
	}
}

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
	InitializeLog();
	logger::info("{} v{}"sv, Plugin::NAME, Plugin::VERSION.string());

	SKSE::Init(a_skse);
	SKSE::AllocTrampoline(35);

	Hooks::Workbench::Install();
	Hooks::Create::Install();
	Hooks::BaseCharge::Install();

	const auto messaging = SKSE::GetMessagingInterface();
	messaging->RegisterListener(&MessageEventCallback);

	return true;
}
