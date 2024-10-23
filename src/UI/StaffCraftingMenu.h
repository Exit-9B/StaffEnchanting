#pragma once

#include "BaseCraftingMenu.h"

namespace UI
{
	class StaffCraftingMenu final : public BaseCraftingMenu<StaffCraftingMenu>
	{
	public:
		static constexpr std::string_view MENU_NAME = "StaffCraftingMenu"sv;

	private:
		// HACK: Values have been chosen to correspond to EnchantConstructMenu so that SkyUI
		// displays appropriate columns.
		enum class FilterFlag
		{
			None = 0x0,
			Recipe = 0x4,
			Staff = 0x1,
			Spell = 0x30,
			Morpholith = 0x40,

			All = 0x7F,
		};

		struct CATEGORY
		{
			enum Category
			{
				Recipe,
				Divider,
				Staff,
				Spell,
				Morpholith,

				TOTAL
			};
		};
		using Category = CATEGORY::Category;

		class CategoryListEntry;
		class SpellEntry;
		class ItemEntry;
		class RecipeEntry;

		friend SpellEntry;
		friend ItemEntry;

	public:
		StaffCraftingMenu() : BaseCraftingMenu("EnchantConstruct") {}

		~StaffCraftingMenu() override;

		[[nodiscard]] static const char* GetMoviePath()
		{
			static const bool useCustomMenu =
				RE::BSResourceNiBinaryStream("Interface/StaffCraftingMenu.swf").good();

			if (useCustomMenu) {
				return "StaffCraftingMenu";
			}
			else {
				return "CraftingMenu";
			}
		}

		static void RegisterFuncs(CallbackProcessor* a_processor);

		void Init();

		bool ProcessUserEvent(const RE::BSFixedString& a_userEvent);

		[[nodiscard]] bool RenderItem3DOnTop() const;

	private:
		static void SetSelectedItem(const RE::FxDelegateArgs& a_params);
		static void SetSelectedCategory(const RE::FxDelegateArgs& a_params);
		static void ChooseItem(const RE::FxDelegateArgs& a_params);
		static void ShowItem3D(const RE::FxDelegateArgs& a_params);

		[[nodiscard]] bool IsCategoryDisabled(Category a_category) const;

		void ClearEffects();
		void ChooseItem(std::uint32_t a_index);
		bool CanSelectEntry(
			const RE::BSTSmartPointer<CategoryListEntry>& a_entry,
			bool a_showNotification = false);
		void CreateItem(const RE::BGSConstructibleObject* a_constructible);

		void UpdateItemPreview(std::unique_ptr<RE::InventoryEntryData>&& a_item);
		void UpdateEnabledEntries(
			FilterFlag a_flags = FilterFlag::All,
			bool a_fullRebuild = false);
		void UpdateEnchantment();
		void UpdateIngredients();
		void UpdateItemList(
			RE::BSTArray<RE::BSTSmartPointer<CategoryListEntry>>& a_entries,
			bool a_fullRebuild = false);

		static void AddSpellIfUsable(
			RE::BSTArray<RE::BSTSmartPointer<CategoryListEntry>>& a_entries,
			const RE::SpellItem* a_spell);

		void ClearSelection();
		void PopulateEntryList(bool a_fullRebuild = false);

		void UpdateInterface();
		void AttemptStaffEnchanting();

	private:
		class Selection
		{
		public:
			[[nodiscard]] constexpr bool Empty() const { return !staff && !morpholith && !spell; }
			void Clear();
			void Toggle(const RE::BSTSmartPointer<CategoryListEntry>& a_entry);

			// members
			RE::BSTSmartPointer<ItemEntry> staff;
			RE::BSTSmartPointer<ItemEntry> morpholith;
			RE::BSTSmartPointer<SpellEntry> spell;
		};

		RE::BSTArray<RE::BSTSmartPointer<CategoryListEntry>> listEntries;
		RE::BSString customName;
		RE::GFxValue inventoryLists;
		RE::GFxValue categoryEntryList;

		Selection selected;

		std::unique_ptr<RE::InventoryEntryData> craftItemPreview;
		std::unique_ptr<RE::ExtraDataList> tempExtraList;
		RE::BSTArray<RE::Effect> createdEffects;
		RE::EnchantmentItem* createdEnchantment{ nullptr };

		std::uint32_t highlightIndex;
		Category currentCategory;
		float chargeAmount{ 0.0f };

		bool exiting{ false };
		bool hasHighlight{ false };
	};
}

#include "StaffCraftingMenu.CategoryListEntry.h"
