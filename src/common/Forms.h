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
			return Forms::LookupForm<RE::BGSKeyword>(0x5EA000, "Update.esm"sv);
		}

		[[nodiscard]] inline auto AllowSoulGems()
		{
			return Forms::LookupForm<RE::BGSKeyword>(0x5EA001, "Update.esm"sv);
		}

		[[nodiscard]] inline auto CreatedStaffName()
		{
			return LookupForm<RE::BGSMessage>(0x802);
		}

		[[nodiscard]] inline auto MenuDescription()
		{
			return LookupForm<RE::BGSMessage>(0x803);
		}

		[[nodiscard]] inline auto HelpStaffEnchantingLong()
		{
			return LookupForm<RE::BGSMessage>(0x804);
		}

		[[nodiscard]] inline auto HelpStaffEnchantingShort()
		{
			return LookupForm<RE::BGSMessage>(0x805);
		}
	}
}
