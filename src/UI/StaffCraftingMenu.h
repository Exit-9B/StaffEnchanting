#pragma once

#include "BaseCraftingMenu.h"

#include "Settings/INISettings.h"

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

		struct SPELL_LEVEL
		{
			enum Spell_Level
			{
				kNovice = 0,
				kApprentice = 1,
				kAdept = 2,
				kExpert = 3,
				kMaster = 4,
			};
		};
		using SpellLevel = SPELL_LEVEL::Spell_Level;

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

#ifndef SKYRIMVR
		void ProcessUpdate(const RE::BSUIMessageData* a_data);
#endif

#ifdef SKYRIMVR
		void AdvanceMovie();

		bool HandleMenuInput(const RE::VrWandTouchpadPositionEvent* a_event);
#endif

		[[nodiscard]] bool RenderItem3DOnTop() const;

	private:
		static void SetSelectedItem(const RE::FxDelegateArgs& a_params);
		static void SetSelectedCategory(const RE::FxDelegateArgs& a_params);
		static void ChooseItem(const RE::FxDelegateArgs& a_params);
		static void ShowItem3D(const RE::FxDelegateArgs& a_params);
		static void CanFadeItemInfo(const RE::FxDelegateArgs& a_params);
		static void EndItemRename(const RE::FxDelegateArgs& a_params);
		static void CloseMenu(const RE::FxDelegateArgs& a_params);
		static void CraftButtonPress(const RE::FxDelegateArgs& a_params);
		static void StartMouseRotation(const RE::FxDelegateArgs& a_params);
		static void StopMouseRotation(const RE::FxDelegateArgs& a_params);
		static void AuxButtonPress(const RE::FxDelegateArgs& a_params);

		[[nodiscard]] bool IsCategoryEmpty(Category a_category) const;

		void ClearEffects();
		void ChooseItem(std::uint32_t a_index);
		bool CanSelectEntry(
			const RE::BSTSmartPointer<CategoryListEntry>& a_entry,
			bool a_showNotification = false);
		void CreateItem(const RE::BGSConstructibleObject* a_constructible);
		void CreateStaff();
		void EditItemName();

		void UpdateItemPreview(std::unique_ptr<RE::InventoryEntryData>&& a_item);
		void UpdateEnabledEntries(
			FilterFlag a_flags = FilterFlag::All,
			bool a_fullRebuild = false);

		[[nodiscard]] static bool CanSetOverrideName(RE::InventoryEntryData* a_item);
		[[nodiscard]] static float GetEntryDataSoulCharge(RE::InventoryEntryData* a_entry);
		[[nodiscard]] static bool MagicEffectHasDescription(RE::EffectSetting* a_effect);
		[[nodiscard]] bool IsSpellValid(const RE::SpellItem* a_spell) const;
		[[nodiscard]] static float CalculateSpellCost(const RE::SpellItem* a_spell);
		[[nodiscard]] static SpellLevel GetSpellLevel(const RE::SpellItem* a_spell);
		[[nodiscard]] static std::int32_t GetSpellHeartstones(const RE::SpellItem* a_spell);
		[[nodiscard]] static float GetDefaultCharge(const RE::SpellItem* a_spell);
		[[nodiscard]] bool CanCraftWithSpell(const RE::SpellItem* a_spell) const;
		[[nodiscard]] static bool IsValidMorpholith(
			const RE::BGSKeywordForm* a_obj,
			const std::vector<const RE::BGSKeyword*>& a_vec);

		void UpdateEnchantmentCharge();
		void UpdateEnchantment();
		void UpdateIngredients();
		void UpdateItemList(
			RE::BSTArray<RE::BSTSmartPointer<CategoryListEntry>>& a_entries,
			bool a_fullRebuild = false);

		void ClearSelection();
		void PopulateEntryList(bool a_fullRebuild = false);

		void UpdateTextElements();

		void TextEntered(const char* a_text);
		void ShowVirtualKeyboard();

		[[nodiscard]] static std::string TranslateFallback(
			const std::string& a_key,
			const std::string& a_fallback)
		{
			std::string result;
			if (!SKSE::Translation::Translate(a_key, result)) {
				result = a_fallback;
			}
			return result;
		}

	private:
		class Selection
		{
		public:
			[[nodiscard]] constexpr bool Empty() const { return !staff && !morpholith && !spell; }
			[[nodiscard]] constexpr bool Complete() const { return staff && morpholith && spell; }
			void Clear();
			void Toggle(const RE::BSTSmartPointer<CategoryListEntry>& a_entry);

			// members
			RE::BSTSmartPointer<ItemEntry> staff;
			RE::BSTSmartPointer<ItemEntry> morpholith;
			RE::BSTSmartPointer<SpellEntry> spell;
		};

		static constexpr std::array<FilterFlag, Category::TOTAL> filters{
			FilterFlag::Recipe,
			FilterFlag::None,
			FilterFlag::Staff,
			FilterFlag::Spell,
			FilterFlag::Morpholith
		};

		std::array<std::string, Category::TOTAL> labels{
			TranslateFallback("$Special"s, "Special"s),
			""s,
			TranslateFallback("$Staff"s, "Staff"s),
			TranslateFallback("$Spell"s, "Spell"s),
			TranslateFallback("$Morpholith"s, "Morpholith"s)
		};

		RE::BSTArray<RE::BSTSmartPointer<CategoryListEntry>> listEntries;
		std::string customName;
		std::string suggestedName;
		RE::GFxValue inventoryLists;
		RE::GFxValue categoryEntryList;

		Selection selected;

		std::unique_ptr<RE::InventoryEntryData> craftItemPreview;
		RE::BSTArray<RE::Effect> createdEffects;
		RE::EnchantmentItem* createdEnchantment{ nullptr };

		std::uint32_t highlightIndex;
		Category currentCategory;
		float chargeAmount{ 0.0f };
		float maxSoulSize{ 0.0f };
		std::int32_t highestMorpholithCount{ 0 };

		bool exiting{ false };
		bool hasHighlight{ false };
#ifdef SKYRIMVR
		bool virtualKeyboardClosing{ false };
#endif

		INISettings ini;
	};
}

#include "StaffCraftingMenu.CategoryListEntry.h"
