#pragma once

namespace SKSE
{
	namespace scaleformExtend
	{
		void CommonItemData(RE::GFxValue& a_fxVal, const RE::TESForm* a_form);

		void StandardItemData(
			RE::GFxValue& a_fxVal,
			const RE::TESForm* a_form,
			const RE::InventoryEntryData* a_entry = nullptr);

		void MagicItemData(
			RE::GFxValue& a_fxVal,
			const RE::TESForm* a_form,
			bool a_extra = true,
			bool a_recursive = true);

		void ItemInfoData(RE::GFxValue& a_fxVal, const RE::InventoryEntryData* a_entry);
	}
}
