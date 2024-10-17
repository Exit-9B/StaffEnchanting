#pragma once

#include "BaseCraftingMenu.h"

namespace UI
{
	class StaffCraftingMenu final : public BaseCraftingMenu<StaffCraftingMenu>
	{
	public:
		static constexpr std::string_view MENU_NAME = "StaffCraftingMenu"sv;

	private:
		enum class FilterFlag
		{
			None = 0x0,
			Staff = 0x1,
			Spell = 0x2,
			Morpholith = 0x4,
			Recipe = 0x8,

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
		friend SpellEntry;
		friend ItemEntry;

	public:
		StaffCraftingMenu() : BaseCraftingMenu("EnchantConstruct") {}

		~StaffCraftingMenu() override;

		// TODO: change to bespoke staff crafting menu
		[[nodiscard]] constexpr static const char* GetMoviePath() { return "CraftingMenu"; }

		static void RegisterFuncs(CallbackProcessor* a_processor);

		void Init();

		bool ProcessUserEvent(const RE::BSFixedString& a_userEvent);

		[[nodiscard]] bool RenderItem3DOnTop() const;

	private:
		static void SetSelectedItem(const RE::FxDelegateArgs& a_params);
		static void SetSelectedCategory(const RE::FxDelegateArgs& a_params);
		static void ChooseItem(const RE::FxDelegateArgs& a_params);
		static void ShowItem3D(const RE::FxDelegateArgs& a_params);

		void ClearEffects();
		void ChooseItem(std::uint32_t a_index);
		bool CanSelectEntry(std::uint32_t a_index, bool a_showNotification = false);

		bool IsFavorite(RE::InventoryEntryData* a_entry);

		void UpdateItemPreview(std::unique_ptr<RE::InventoryEntryData>&& a_item);
		void UpdateEnabledEntries(std::uint32_t a_flags = 0x7F, bool a_fullRebuild = false);
		void UpdateEnchantment();
		void UpdateIngredients();
		void UpdateItemList(
			RE::BSTArray<RE::BSTSmartPointer<CategoryListEntry>>& a_entries,
			bool a_fullRebuild = false);

		static void AddSpellIfUsable(
			RE::BSTArray<RE::BSTSmartPointer<CategoryListEntry>>& a_entries,
			const RE::SpellItem* a_spell);

		void ClearEntryList();
		void PopulateEntryList();

		void UpdateInterface();

	private:
		class Selection
		{
		public:
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
