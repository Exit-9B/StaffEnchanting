#include "BaseCraftingMenu.h"

#include "RE/Misc.h"

namespace UI
{
	template <typename Impl>
	inline BaseCraftingMenu<Impl>::BaseCraftingMenu(const char* a_movieFrame)
		: movieFrame{ a_movieFrame }
	{
		menuFlags = {
			RE::UI_MENU_FLAGS::kDontHideCursorWhenTopmost,
			RE::UI_MENU_FLAGS::kInventoryItemMenu,
			RE::UI_MENU_FLAGS::kUpdateUsesCursor,
			RE::UI_MENU_FLAGS::kDisablePauseMenu,
			RE::UI_MENU_FLAGS::kUsesMenuContext
		};
		const auto scaleformManager = RE::BSScaleformManager::GetSingleton();
		assert(scaleformManager);
		[[maybe_unused]] const bool movieLoaded = scaleformManager->LoadMovie(
			this,
			uiMovie,
			GetImpl()->GetMoviePath(),
			RE::GFxMovieView::ScaleModeType::kExactFit);
		assert(movieLoaded);
		assert(uiMovie);

		depthPriority = 0;
		inputContext = Context::kItemMenu;
		RE::SendHUDMessage::PushHudMode("InventoryMode");

		const auto controlMap = RE::ControlMap::GetSingleton();
		assert(controlMap);
		controlMap->StoreControls();
		controlMap->ToggleControls(RE::UserEvents::USER_EVENT_FLAG::kVATS, false, false);
		controlMap->ToggleControls(RE::UserEvents::USER_EVENT_FLAG::kLooking, false, false);
		controlMap->ToggleControls(RE::UserEvents::USER_EVENT_FLAG::kPOVSwitch, false, false);
		controlMap->ToggleControls(RE::UserEvents::USER_EVENT_FLAG::kWheelZoom, false, false);

		const auto uiBlurManager = RE::UIBlurManager::GetSingleton();
		assert(uiBlurManager);
		uiBlurManager->IncrementBlurCount();
	}

	template <typename Impl>
	inline BaseCraftingMenu<Impl>::~BaseCraftingMenu()
	{
		const auto controlMap = RE::ControlMap::GetSingleton();
		assert(controlMap);
		controlMap->LoadStoredControls();

		RE::SendHUDMessage::PopHudMode("InventoryMode");

		const auto uiBlurManager = RE::UIBlurManager::GetSingleton();
		assert(uiBlurManager);
		uiBlurManager->DecrementBlurCount();

		itemCard.reset();
		bottomBar.reset();

		const auto inventory3D = RE::Inventory3DManager::GetSingleton();
		assert(inventory3D);
		if (inventory3D->state) {
			inventory3D->End3D();
		}

		const auto eventSource = RE::ScriptEventSourceHolder::GetSingleton();
		assert(eventSource);
		eventSource->RemoveEventSink(this);
	}

	template <typename Impl>
	inline void BaseCraftingMenu<Impl>::Accept(CallbackProcessor* a_processor)
	{
		Impl::RegisterFuncs(a_processor);
	}

	template <typename Impl>
	inline RE::UI_MESSAGE_RESULTS BaseCraftingMenu<Impl>::ProcessMessage(RE::UIMessage& a_message)
	{
		RE::UI_MESSAGE_RESULTS result = RE::UI_MESSAGE_RESULTS::kHandled;

		switch (a_message.type.get()) {
		case RE::UI_MESSAGE_TYPE::kUpdate:
			Update(static_cast<RE::BSUIMessageData*>(a_message.data));
			break;

		case RE::UI_MESSAGE_TYPE::kShow:
			Show();
			break;

		case RE::UI_MESSAGE_TYPE::kUserEvent:
			UserEvent(static_cast<RE::BSUIMessageData*>(a_message.data));
			break;

		case RE::UI_MESSAGE_TYPE::kInventoryUpdate:
			InventoryUpdate(static_cast<RE::InventoryUpdateData*>(a_message.data));
			break;

		default:
			result = RE::IMenu::ProcessMessage(a_message);
			break;
		}

		return result;
	}

	template <typename Impl>
	inline void BaseCraftingMenu<Impl>::Update(const RE::BSUIMessageData* a_data)
	{
		GetImpl()->ProcessUpdate(a_data);
	}

	template <typename Impl>
	inline void BaseCraftingMenu<Impl>::Show()
	{
		RE::TESObjectREFRPtr furnitureRef;
		const auto playerRef = RE::PlayerCharacter::GetSingleton();
		if (playerRef && playerRef->currentProcess) {
			furnitureRef = playerRef->currentProcess->GetOccupiedFurniture().get();
		}
		const auto furnitureObj = furnitureRef ? furnitureRef->GetBaseObject() : nullptr;
		workbench = furnitureObj ? furnitureObj->As<RE::TESFurniture>() : nullptr;

		// Common crafting setup
		assert(uiMovie);
		uiMovie->GetVariable(&menu, "Menu");
		menu.Invoke("gotoAndStop", std::to_array<RE::GFxValue>({ movieFrame }));
		menu.Invoke("Initialize");

		uiMovie->GetVariable(&itemList, "Menu.ItemList");
		uiMovie->GetVariable(&itemInfo, "Menu.ItemInfo");
		uiMovie->GetVariable(&bottomBarInfo, "Menu.BottomBarInfo");
		uiMovie->GetVariable(&additionalDescription, "Menu.AdditionalDescription");
		uiMovie->GetVariable(&menuName, "Menu.MenuName");
		uiMovie->GetVariable(&buttonText, "Menu.ButtonText");

		itemCard = std::make_unique<RE::ItemCard>(uiMovie.get());
		bottomBar = std::make_unique<RE::BottomBar>(uiMovie.get());

		if (buttonText.IsArray()) {
			buttonText.SetElement(Button::Select, *"sSelect"_gs);
			buttonText.SetElement(Button::Exit, *"sExit"_gs);
			buttonText.SetElement(Button::Craft, *"sCraft"_gs);
		}

		menu.Invoke("UpdateButtonText");
		const auto inventory3D = RE::Inventory3DManager::GetSingleton();
		assert(inventory3D);
		inventory3D->Begin3D(1);

		const auto eventSource = RE::ScriptEventSourceHolder::GetSingleton();
		assert(eventSource);
		eventSource->AddEventSink(this);

		GetImpl()->Init();
	}

	template <typename Impl>
	inline void BaseCraftingMenu<Impl>::ExitCraftingWorkbench()
	{
		const auto uiMessageQueue = RE::UIMessageQueue::GetSingleton();
		assert(uiMessageQueue);
		uiMessageQueue->AddMessage(Impl::MENU_NAME, RE::UI_MESSAGE_TYPE::kHide, nullptr);

		const auto playerRef = RE::PlayerCharacter::GetSingleton();
		assert(playerRef);
		assert(playerRef->currentProcess);
		const auto furnitureRef = playerRef->currentProcess->GetOccupiedFurniture().get();
		if (!furnitureRef ||
			RE::BSFurnitureMarkerNode::GetNumFurnitureMarkers(furnitureRef->Get3D())) {
			if (playerRef->actorState1.sitSleepState == RE::SIT_SLEEP_STATE::kIsSitting) {
				playerRef->InitiateGetUpPackage();
			}
		}
		else {
			playerRef->currentProcess->ClearFurniture();
		}
	}

	template <typename Impl>
	inline void BaseCraftingMenu<Impl>::UserEvent(const RE::BSUIMessageData* a_data)
	{
		assert(a_data);
		const RE::BSFixedString& userEvent = a_data->fixedStr;

		const auto userEvents = RE::UserEvents::GetSingleton();
		assert(userEvents);
		if (userEvent == userEvents->cancel) {
			if (!GetImpl()->ProcessUserEvent(userEvent)) {
				ExitCraftingWorkbench();
			}
		}
		else {
			GetImpl()->ProcessUserEvent(userEvent);
		}
	}

	template <typename Impl>
	inline void BaseCraftingMenu<Impl>::InventoryUpdate(
		[[maybe_unused]] const RE::InventoryUpdateData* a_data)
	{
#if 0
		assert(a_data);
		const auto handle = a_data->unk10;
		static const REL::Relocation<RE::RefHandle*> playerHandle{ REL::ID(403520) };
		if (handle && handle == *playerHandle && !a_data->unk18) {
			;
		}
#endif
	}

	template <typename Impl>
	inline void BaseCraftingMenu<Impl>::PostDisplay()
	{
		const auto inventory3D = RE::Inventory3DManager::GetSingleton();
		assert(inventory3D);

		if (GetImpl()->RenderItem3DOnTop()) {
			uiMovie->Display();
			inventory3D->Render();
		}
		else {
			inventory3D->Render();
			uiMovie->Display();
		}
	}

	template <typename Impl>
	inline RE::BSEventNotifyControl BaseCraftingMenu<Impl>::ProcessEvent(
		const RE::TESFurnitureEvent* a_event,
		[[maybe_unused]] RE::BSTEventSource<RE::TESFurnitureEvent>* a_eventSource)
	{
		using enum RE::BSEventNotifyControl;
		assert(a_event);
		assert(a_event->actor);
		if (a_event->type == RE::TESFurnitureEvent::FurnitureEventType::kExit &&
			a_event->actor->IsPlayerRef()) {
			const auto uiMessageQueue = RE::UIMessageQueue::GetSingleton();
			assert(uiMessageQueue);
			uiMessageQueue->AddMessage(Impl::MENU_NAME, RE::UI_MESSAGE_TYPE::kHide, nullptr);
		}

		return kContinue;
	}

	template <typename Impl>
	inline void BaseCraftingMenu<Impl>::SetMenuDescription(const char* a_description)
	{
		RE::GFxValue menuDescription;
		if (menu.GetMember("MenuDescription", &menuDescription)) {
			menuDescription.SetMember("text", a_description);
		}
	}

	template <typename Impl>
	inline void BaseCraftingMenu<Impl>::UpdateItemCard(const RE::InventoryEntryData* a_item)
	{
		if (!itemInfo.IsObject()) {
			return;
		}

		auto& obj = itemCard->obj;
		if (a_item) {
			itemCard->SetItem(a_item, false);
		}
		else {
			obj.SetMember("type", nullptr);
		}

		if (!showItemCardName && obj.HasMember("name")) {
			obj.DeleteMember("name");
		}

		itemInfo.SetMember("itemInfo", obj);
	}

	template <typename Impl>
	inline void BaseCraftingMenu<Impl>::UpdateItemCard(const RE::TESForm* a_form)
	{
		if (!itemInfo.IsObject()) {
			return;
		}

		auto& obj = itemCard->obj;
		if (a_form) {
			itemCard->SetForm(a_form);
		}
		else {
			obj.SetMember("type", nullptr);
		}

		if (!showItemCardName && obj.HasMember("name")) {
			obj.DeleteMember("name");
		}

		itemInfo.SetMember("itemInfo", obj);
	}

	template <typename Impl>
	inline void BaseCraftingMenu<Impl>::UpdateBottomBar(RE::ActorValue a_skill)
	{
		if (bottomBarInfo.IsObject()) {
			const auto playerRef = RE::PlayerCharacter::GetSingleton();
			assert(playerRef);
			const auto playerSkills = playerRef->skills;
			assert(playerSkills);
			assert(playerSkills->data);
			const auto& skillData = playerSkills->data->skills[util::to_underlying(a_skill) - 6];
			const float xp = skillData.xp;
			const float levelThreshold = skillData.levelThreshold;

			bottomBarInfo.Invoke(
				"UpdateCraftingInfo",
				std::to_array<RE::GFxValue>(
					{ RE::ActorValueInfo::GetActorValueName(a_skill),
					  playerRef->GetActorValue(a_skill),
					  (xp / levelThreshold) * 100.0 }));
		}
	}
}
