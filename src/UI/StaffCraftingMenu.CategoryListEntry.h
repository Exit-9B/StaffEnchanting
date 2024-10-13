#pragma once

#include "StaffCraftingMenu.h"

namespace UI
{
	class StaffCraftingMenu::CategoryListEntry : public RE::BSIntrusiveRefCounted
	{
	public:
		CategoryListEntry(FilterFlag a_filterFlag) : filterFlag{ a_filterFlag } {}

		virtual void ShowInItemCard([[maybe_unused]] StaffCraftingMenu* a_menu) const {}

		virtual void ShowItem3D([[maybe_unused]] bool a_show) const
		{
			const auto inventory3D = RE::Inventory3DManager::GetSingleton();
			assert(inventory3D);
			inventory3D->Clear3D();
		}

		[[nodiscard]] virtual const char* GetName() const = 0;

		virtual void SetupEntryObjectByType([[maybe_unused]] RE::GFxValue& a_entryObj) const {}

		void SetupEntryObject(RE::GFxValue& a_entryObj) const;

		// members
		FilterFlag filterFlag;
		bool selected{ false };
		bool enabled{ true };
	};

	class StaffCraftingMenu::SpellEntry final : public CategoryListEntry
	{
	public:
		explicit SpellEntry(const RE::SpellItem* a_spell)
			: CategoryListEntry(FilterFlag::Spell),
			  data{ a_spell }
		{
		}

		void ShowInItemCard(StaffCraftingMenu* a_menu) const override;

		void ShowItem3D(bool a_show) const override;

		[[nodiscard]] const char* GetName() const override;

		void SetupEntryObjectByType(RE::GFxValue& a_entryObj) const override;

		// members
		const RE::SpellItem* data;
	};

	class StaffCraftingMenu::ItemEntry final : public CategoryListEntry
	{
	public:
		ItemEntry(std::unique_ptr<RE::InventoryEntryData>&& a_item, FilterFlag a_filterFlag)
			: CategoryListEntry(a_filterFlag),
			  data{ std::move(a_item) }
		{
		}

		void ShowInItemCard(StaffCraftingMenu* a_menu) const override;

		void ShowItem3D(bool a_show) const override;

		[[nodiscard]] const char* GetName() const override;

		void SetupEntryObjectByType(RE::GFxValue& a_entryObj) const override;

		// members
		std::unique_ptr<RE::InventoryEntryData> data;
	};
}
