#pragma once

namespace Forms
{
	template <typename T>
	[[nodiscard]] inline T* LookupForm(RE::FormID a_formID, SKSE::stl::zstring a_modName)
	{
		const auto dataHandler = RE::TESDataHandler::GetSingleton();
		return dataHandler ? dataHandler->LookupForm<T>(a_formID, a_modName) : nullptr;
	}

	namespace StaffEnchanting
	{
		template <typename T>
		[[nodiscard]] inline T* LookupForm(RE::FormID a_formID)
		{
			return Forms::LookupForm<T>(a_formID, "StaffEnchanting.esp"sv);
		}

		[[nodiscard]] inline auto DisallowHeartStones()
		{
			return LookupForm<RE::BGSKeyword>(0x800);
		}

		[[nodiscard]] inline auto AllowSoulGems()
		{
			return LookupForm<RE::BGSKeyword>(0x801);
		}

		[[nodiscard]] inline auto MenuDescription()
		{
			return LookupForm<RE::BGSMessage>(0x802);
		}

		[[nodiscard]] inline auto CreatedStaffName()
		{
			return LookupForm<RE::BGSMessage>(0x803);
		}
	}
}
