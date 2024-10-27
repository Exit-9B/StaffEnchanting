#pragma once

namespace UI
{
	template <typename Impl>
	class BaseCraftingMenu : public RE::IMenu, public RE::BSTEventSink<RE::TESFurnitureEvent>
	{
	public:
		static void Register()
		{
			if (const auto ui = RE::UI::GetSingleton()) {
				ui->Register(
					Impl::MENU_NAME,
					+[]() -> SKSE::stl::owner<RE::IMenu*>
					{
						return new Impl();
					});
			}
		}

	protected:
		BaseCraftingMenu(const char* a_movieFrame);
		~BaseCraftingMenu();

	public:
		BaseCraftingMenu(const BaseCraftingMenu&) = delete;
		BaseCraftingMenu(BaseCraftingMenu&&) = delete;
		BaseCraftingMenu& operator=(const BaseCraftingMenu&) = delete;
		BaseCraftingMenu& operator=(BaseCraftingMenu&&) = delete;

	public:
		// FxDelegateHandler
		void Accept(CallbackProcessor* a_processor) final;

		// IMenu
		RE::UI_MESSAGE_RESULTS ProcessMessage(RE::UIMessage& a_message) final;
		void PostDisplay() final;

		// BSTEventSink<TESFurnitureEvent>
		RE::BSEventNotifyControl ProcessEvent(
			const RE::TESFurnitureEvent* a_event,
			RE::BSTEventSource<RE::TESFurnitureEvent>* a_eventSource) final;

	protected:
		struct BUTTONS
		{
			enum Button
			{
				Select = 0,
				Exit = 1,
				Aux = 2,
				Craft = 3,
			};
		};
		using Button = BUTTONS::Button;

		void SetMenuDescription(const char* a_description);

		void UpdateItemCard(const RE::InventoryEntryData* a_item);
		void UpdateItemCard(const RE::TESForm* a_form);
		void UpdateItemCard(std::nullptr_t) { UpdateItemCard((RE::InventoryEntryData*)nullptr); }

		void UpdateBottomBar(RE::ActorValue a_skill);

	private:
		Impl* GetImpl() { return static_cast<Impl*>(this); }
		const Impl* GetImpl() const { return static_cast<const Impl*>(this); }

		void Update(const RE::BSUIMessageData* a_data);
		void Show();
		void UserEvent(const RE::BSUIMessageData* a_data);
		void InventoryUpdate(const RE::InventoryUpdateData* a_data);

		static void ExitCraftingWorkbench();

	protected:
		const char* movieFrame;
		RE::TESFurniture* workbench;

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
	};
}

#include "BaseCraftingMenu.inl"
