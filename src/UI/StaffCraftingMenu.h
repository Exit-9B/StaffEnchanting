#pragma once

namespace UI
{
	class StaffCraftingMenu final :
		public RE::IMenu,
		public RE::BSTEventSink<RE::TESFurnitureEvent>
	{
	public:
		static constexpr std::string_view MENU_NAME = "StaffCraftingMenu"sv;

		static void Register()
		{
			if (const auto ui = RE::UI::GetSingleton()) {
				ui->Register(
					MENU_NAME,
					+[]() -> SKSE::stl::owner<RE::IMenu*>
					{
						return new StaffCraftingMenu();
					});
			}
		}

		StaffCraftingMenu();
		StaffCraftingMenu(const StaffCraftingMenu&) = delete;
		StaffCraftingMenu(StaffCraftingMenu&&) = delete;

		~StaffCraftingMenu();

		StaffCraftingMenu& operator=(const StaffCraftingMenu&) = delete;
		StaffCraftingMenu& operator=(StaffCraftingMenu&&) = delete;

		// FxDelegateHandler
		void Accept(CallbackProcessor* a_processor) override;

		friend struct StaffCraftingMenu_Callbacks;

		// IMenu
		RE::UI_MESSAGE_RESULTS ProcessMessage(RE::UIMessage& a_message) override;

		void PostDisplay() override;

		// BSTEventSink<TESFurnitureEvent>
		RE::BSEventNotifyControl ProcessEvent(
			const RE::TESFurnitureEvent* a_event,
			RE::BSTEventSource<RE::TESFurnitureEvent>* a_eventSource);

	private:
		enum class FilterFlag
		{
			None = 0x0,
			Staff = 0x1,
			Spell = 0x2,
			Morpholith = 0x4,

			All = 0x7F,
		};

		enum class Category
		{
			None,
			Staff,
			Spell,
			Morpholith,
		};

		class CategoryListEntry : public RE::BSIntrusiveRefCounted
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

		class SpellEntry final : public CategoryListEntry
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
		friend SpellEntry;

		class ItemEntry final : public CategoryListEntry
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
		friend ItemEntry;

	private:
		void ChooseItem(std::uint32_t a_index);

		void Update(const RE::BSUIMessageData* a_data, RE::UI_MESSAGE_RESULTS& a_result);
		void Show(RE::UI_MESSAGE_RESULTS& a_result);
		void UserEvent(const RE::BSUIMessageData* a_data, RE::UI_MESSAGE_RESULTS& a_result);
		void InventoryUpdate(
			const RE::InventoryUpdateData* a_data,
			RE::UI_MESSAGE_RESULTS& a_result);

		static void ExitCraftingWorkbench();

		void SetMenuDescription(const char* a_description);

		void DeselectAll();
		void UpdateItemPreview(std::unique_ptr<RE::InventoryEntryData> a_item);
		void UpdateItemCard(const RE::InventoryEntryData* a_item);
		void UpdateItemCard(const RE::TESForm* a_form);
		void UpdateItemCard(std::nullptr_t) { UpdateItemCard((RE::InventoryEntryData*)nullptr); }
		void UpdateEnabledEntries(std::uint32_t a_flags = 0x7F, bool a_fullRebuild = false);
		void UpdateAdditionalDescription();
		void UpdateItemList(
			RE::BSTArray<RE::BSTSmartPointer<CategoryListEntry>>& a_entries,
			bool a_fullRebuild = false);

		static void AddSpellIfUsable(
			RE::BSTArray<RE::BSTSmartPointer<CategoryListEntry>>& a_entries,
			const RE::SpellItem* a_spell);

		void ClearEntryList();
		void PopulateEntryList();

		void UpdateInterface();
		void UpdateBottomBar();

	private:
		const RE::TESFurniture* furniture{ nullptr };

		// stage elements
		std::unique_ptr<RE::ItemCard> itemCard;
		std::unique_ptr<RE::BottomBar> bottomBar;
		RE::GFxValue menu;
		RE::GFxValue itemList;
		RE::GFxValue itemEntryList;
		RE::GFxValue itemInfo;
		RE::GFxValue bottomBarInfo;
		RE::GFxValue additionalDescription;
		RE::GFxValue menuName;
		RE::GFxValue buttonText;

		bool showItemCardName{ true };

		// enchant menu data
		RE::BSTArray<RE::BSTSmartPointer<CategoryListEntry>> listEntries;
		RE::BSString customName;
		RE::GFxValue inventoryLists;
		RE::GFxValue categoryEntryList;

		// selections
		RE::BSTSmartPointer<ItemEntry> selectedItem;
		RE::BSTSmartPointer<SpellEntry> selectedSpell;

		std::unique_ptr<RE::InventoryEntryData> craftItemPreview;
		std::uint32_t highlightIndex;
		Category currentCategory;

		// input state
		bool exiting{ false };
		bool hasHighlight{ false };
	};
}
