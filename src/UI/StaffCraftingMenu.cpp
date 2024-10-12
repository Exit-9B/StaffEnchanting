#include "StaffCraftingMenu.h"
#include "StaffCraftingMenu_Callbacks.h"

#include "RE/Misc.h"
#include "RE/Offset.h"

namespace UI
{
	StaffCraftingMenu::StaffCraftingMenu()
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
		// TODO: change to bespoke staff crafting menu
		const bool movieLoaded = scaleformManager->LoadMovie(this, uiMovie, "CraftingMenu");
		assert(movieLoaded);
		assert(uiMovie);

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

	StaffCraftingMenu::~StaffCraftingMenu()
	{
		const auto controlMap = RE::ControlMap::GetSingleton();
		assert(controlMap);
		controlMap->LoadStoredControls();

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

	void StaffCraftingMenu::Accept(CallbackProcessor* a_processor)
	{
		StaffCraftingMenu_Callbacks::Register(a_processor);
	}

	RE::UI_MESSAGE_RESULTS StaffCraftingMenu::ProcessMessage(RE::UIMessage& a_message)
	{
		RE::UI_MESSAGE_RESULTS result = RE::UI_MESSAGE_RESULTS::kHandled;

		switch (a_message.type.get()) {
		case RE::UI_MESSAGE_TYPE::kUpdate:
			Update(static_cast<RE::BSUIMessageData*>(a_message.data), result);
			break;

		case RE::UI_MESSAGE_TYPE::kShow:
			Show(result);
			break;

		case RE::UI_MESSAGE_TYPE::kUserEvent:
			UserEvent(static_cast<RE::BSUIMessageData*>(a_message.data), result);
			break;

		case RE::UI_MESSAGE_TYPE::kInventoryUpdate:
			InventoryUpdate(static_cast<RE::InventoryUpdateData*>(a_message.data), result);
			break;

		default:
			result = RE::IMenu::ProcessMessage(a_message);
			break;
		}

		return result;
	}

	void StaffCraftingMenu::Update(
		const RE::BSUIMessageData* a_data,
		[[maybe_unused]] RE::UI_MESSAGE_RESULTS& a_result)
	{
		if (!a_data) {
			return;
		}

		const auto& ev = a_data->fixedStr;
		if (ev != "VirtualKeyboardCancelled"sv && ev != "VirtualKeyboardDone"sv) {
			return;
		}

		// TODO: virtual keyboard events
	}

	void StaffCraftingMenu::Show([[maybe_unused]] RE::UI_MESSAGE_RESULTS& a_result)
	{
		RE::TESObjectREFRPtr furnitureRef;
		const auto playerRef = RE::PlayerCharacter::GetSingleton();
		if (playerRef && playerRef->currentProcess) {
			furnitureRef = playerRef->currentProcess->GetOccupiedFurniture().get();
		}
		const auto furnitureObj = furnitureRef ? furnitureRef->GetBaseObject() : nullptr;
		furniture = furnitureObj ? furnitureObj->As<RE::TESFurniture>() : nullptr;

		// Common crafting setup
		assert(uiMovie);
		uiMovie->GetVariable(&menu, "Menu");
		menu.Invoke("gotoAndStop", std::to_array<RE::GFxValue>({ "EnchantConstruct" }));
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
			buttonText.SetElement(0, *"sSelect"_gs);
			buttonText.SetElement(1, *"sExit"_gs);
			buttonText.SetElement(3, *"sCraft"_gs);
		}

		menu.Invoke("UpdateButtonText");
		const auto inventory3D = RE::Inventory3DManager::GetSingleton();
		assert(inventory3D);
		inventory3D->Begin3D(1);

		const auto eventSource = RE::ScriptEventSourceHolder::GetSingleton();
		assert(eventSource);
		eventSource->AddEventSink(this);

		// Workbench-specific setup
		// TODO: localization
		SetMenuDescription(
			"Staff Enchanting: Combine a Staff, Spell, and Morpholith to create magic staves");
		menu.SetMember("bCanCraft", true);
		menu.GetMember("CategoryList", &inventoryLists);
		if (inventoryLists.IsObject()) {
			std::array<const char*, 5> labels{ "", "", "Staff", "Spell", "Morpholith" };

			std::array<FilterFlag, 5> filters{
				FilterFlag::None,
				FilterFlag::None,
				FilterFlag::Staff,
				FilterFlag::Spell,
				FilterFlag::Morpholith
			};

			std::array<RE::GFxValue, 15> categories;

			for (const auto i : std::views::iota(0ull, 5ull)) {
				categories[i * 3 + 0] = labels[i];
				categories[i * 3 + 1] = filters[i];
				categories[i * 3 + 2] = true;
			}

			inventoryLists.Invoke("SetCategoriesList", categories);

			RE::GFxValue categoriesList;
			inventoryLists.GetMember("CategoriesList", &categoriesList);
			if (categoriesList.IsObject()) {
				categoriesList.GetMember("entryList", &categoryEntryList);
			}

			if (categoryEntryList.IsArray()) {
				RE::GFxValue divider;
				if (categoryEntryList.GetElement(1, &divider)) {
					divider.SetMember("divider", true);
				}
				inventoryLists.Invoke("InvalidateListData");
			}

			PopulateEntryList();
		}

		UpdateInterface();
		UpdateBottomBar();
	}

	void StaffCraftingMenu::PopulateEntryList()
	{
		ClearEntryList();

		const auto playerRef = RE::PlayerCharacter::GetSingleton();
		assert(playerRef);

		auto inventory = playerRef->GetInventory(
			[](RE::TESBoundObject& baseObj) -> bool
			{
				if (const auto weap = baseObj.As<RE::TESObjectWEAP>()) {
					return weap->IsStaff();
				}
				return false;
			});

		for (auto& [baseObj, extra] : inventory) {
			auto& [count, entry] = extra;
			listEntries.push_back(RE::BSTSmartPointer(
				RE::make_smart<ItemEntry>(std::move(entry), FilterFlag::Staff)));
		}

		const auto playerBase = playerRef->GetActorBase();
		assert(playerBase);
		const auto spellData = playerBase->actorEffects;
		assert(spellData);
		for (const auto spell : std::span(spellData->spells, spellData->numSpells)) {
			AddSpellIfUsable(listEntries, spell);
		}

		for (const auto spell : playerRef->addedSpells) {
			AddSpellIfUsable(listEntries, spell);
		}

		highlightIndex = listEntries.empty()
			? 0
			: std::min(highlightIndex, listEntries.size() - 1);

		if (itemList.IsObject()) {
			itemList.GetMember("entryList", &itemEntryList);
			itemEntryList.SetArraySize(0);
			for (const auto& listEntry : listEntries) {
				RE::GFxValue itemEntry;
				uiMovie->CreateObject(&itemEntry);
				listEntry->SetupEntryObject(itemEntry);
				itemEntryList.PushBack(itemEntry);
			}
		}

		// TBD: constructible object recipes

		UpdateEnabledEntries();
		UpdateAdditionalDescription();
	}

	void StaffCraftingMenu::ClearEntryList()
	{
		listEntries.clear();
		DeselectAll();
		UpdateItemPreview(nullptr);
		UpdateEnabledEntries();
		UpdateAdditionalDescription();
		UpdateItemList(listEntries, false);
		// createEffectFunctor.ClearEffects();
	}

	void StaffCraftingMenu::DeselectAll()
	{
		if (selectedItem) {
			selectedItem->selected = false;
		}

		selectedItem.reset();

		if (selectedSpell) {
			selectedSpell->selected = false;
		}

		selectedSpell.reset();
	}

	void StaffCraftingMenu::UpdateItemPreview(std::unique_ptr<RE::InventoryEntryData> a_item)
	{
		const auto inventory3D = RE::Inventory3DManager::GetSingleton();
		assert(inventory3D);

		if (craftItemPreview) {
			inventory3D->Clear3D();
		}

		if (a_item != craftItemPreview) {
			craftItemPreview = std::move(a_item);
		}

		if (craftItemPreview) {
			UpdateItemCard(craftItemPreview.get());
			inventory3D->UpdateItem3D(craftItemPreview.get());
		}

		customName.clear();
		UpdateInterface();
	}

	void StaffCraftingMenu::UpdateItemCard(const RE::InventoryEntryData* a_item)
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

	void StaffCraftingMenu::UpdateItemCard(const RE::TESForm* a_form)
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

	void StaffCraftingMenu::UpdateEnabledEntries(std::uint32_t a_flags, bool a_fullRebuild)
	{
		(void)a_flags;
		// TODO: see 51450

		if (menu.IsObject()) {
			menu.Invoke("UpdateItemList", std::to_array<RE::GFxValue>({ a_fullRebuild }));
		}
	}

	void StaffCraftingMenu::UpdateAdditionalDescription()
	{
		// TODO: see 51457
	}

	void StaffCraftingMenu::UpdateItemList(
		RE::BSTArray<RE::BSTSmartPointer<CategoryListEntry>>& a_entries,
		bool a_fullRebuild)
	{
		if (itemList.IsObject() && itemEntryList.IsArray()) {
			for (const auto i :
				 std::views::iota(0u, std::min(itemEntryList.GetArraySize(), a_entries.size()))) {

				RE::GFxValue entryObj;
				itemEntryList.GetElement(i, &entryObj);

				assert(a_entries[i]);
				a_entries[i]->SetupEntryObject(entryObj);
			}
		}

		menu.Invoke("UpdateItemList", std::to_array<RE::GFxValue>({ a_fullRebuild }));
	}

	void StaffCraftingMenu::AddSpellIfUsable(
		RE::BSTArray<RE::BSTSmartPointer<CategoryListEntry>>& a_entries,
		const RE::SpellItem* a_spell)
	{
		assert(a_spell);

		const auto castingType = a_spell->GetCastingType();
		if (!(castingType == RE::MagicSystem::CastingType::kFireAndForget ||
			  castingType == RE::MagicSystem::CastingType::kConcentration)) {
			return;
		}

		const auto delivery = a_spell->GetDelivery();
		if (!(delivery == RE::MagicSystem::Delivery::kAimed ||
			  delivery == RE::MagicSystem::Delivery::kTargetActor ||
			  delivery == RE::MagicSystem::Delivery::kTargetLocation)) {
			return;
		}

		a_entries.push_back(RE::make_smart<SpellEntry>(a_spell));
	}

	void StaffCraftingMenu::UpdateInterface()
	{
		// TODO: see 51459
		if (craftItemPreview) {
			UpdateItemCard(craftItemPreview.get());
		}
		else if (hasHighlight && highlightIndex < listEntries.size()) {
			listEntries[highlightIndex]->ShowInItemCard(this);
		}
		else {
			UpdateItemCard(nullptr);
		}
	}

	void StaffCraftingMenu::UpdateBottomBar()
	{
		if (bottomBarInfo.IsObject()) {
			const auto playerRef = RE::PlayerCharacter::GetSingleton();
			assert(playerRef);
			const auto playerSkills = playerRef->skills;
			assert(playerSkills);
			assert(playerSkills->data);
			const auto& skillData =
				playerSkills->data
					->skills[RE::PlayerCharacter::PlayerSkills::Data::Skill::kEnchanting];
			const float xp = skillData.xp;
			const float levelThreshold = skillData.levelThreshold;

			bottomBarInfo.Invoke(
				"UpdateCraftingInfo",
				std::to_array<RE::GFxValue>(
					{ RE::GetActorValueName(RE::ActorValue::kEnchanting),
					  playerRef->GetActorValue(RE::ActorValue::kEnchanting),
					  (xp / levelThreshold) * 100.0 }));
		}
	}

	void StaffCraftingMenu::UserEvent(
		const RE::BSUIMessageData* a_data,
		[[maybe_unused]] RE::UI_MESSAGE_RESULTS& a_result)
	{
		assert(a_data);
		const RE::BSFixedString& userEvent = a_data->fixedStr;

		// TBD: magnitude slider

		const auto userEvents = RE::UserEvents::GetSingleton();
		assert(userEvents);
		if (userEvent == userEvents->cancel) {
			// TODO: skip confirmation if nothing selected
			if (exiting) {
				ExitCraftingWorkbench();
				return;
			}

			const auto msgBoxData = new RE::MessageBoxData();
			msgBoxData->bodyText = *"sQuitEnchanting"_gs;
			msgBoxData->buttonText.push_back(*"sYes"_gs);
			msgBoxData->buttonText.push_back(*"sNo"_gs);

			struct MenuExitCallback : public RE::IMessageBoxCallback
			{
				StaffCraftingMenu* menu;

				MenuExitCallback(StaffCraftingMenu* a_menu) : menu{ a_menu } {}

				void Run(Message a_msg) override
				{
					if (a_msg != Message::kUnk0) {
						menu->exiting = false;
					}
					else {
						const auto uiMessageQueue = RE::UIMessageQueue::GetSingleton();
						if (uiMessageQueue) {
							const auto interfaceStrings = RE::InterfaceStrings::GetSingleton();
							const auto userEvents = RE::UserEvents::GetSingleton();
							assert(interfaceStrings);
							assert(userEvents);

							const auto msgData = static_cast<RE::BSUIMessageData*>(
								RE::CreateUIMessageData(interfaceStrings->bsUIMessageData));
							assert(msgData);
							msgData->fixedStr = userEvents->cancel;

							uiMessageQueue
								->AddMessage(MENU_NAME, RE::UI_MESSAGE_TYPE::kUserEvent, msgData);
						}
					}
				}
			};

			msgBoxData->callback = RE::make_smart<MenuExitCallback>(this);
			msgBoxData->unk38 = 25;
			msgBoxData->unk48 = 4;

			msgBoxData->QueueMessage();
			exiting = true;
			a_result = RE::UI_MESSAGE_RESULTS::kHandled;
		}
		else if (userEvent == userEvents->xButton) {
			// TODO: craft
		}
		else if (userEvent == userEvents->yButton) {
			// TODO: edit name
		}
	}

	void StaffCraftingMenu::ExitCraftingWorkbench()
	{
		const auto uiMessageQueue = RE::UIMessageQueue::GetSingleton();
		assert(uiMessageQueue);
		uiMessageQueue->AddMessage(MENU_NAME, RE::UI_MESSAGE_TYPE::kHide, nullptr);

		const auto playerRef = RE::PlayerCharacter::GetSingleton();
		assert(playerRef);
		assert(playerRef->currentProcess);
		const auto furnitureRef = playerRef->currentProcess->GetOccupiedFurniture().get();
		if (!furnitureRef || RE::GetFurnitureMarkerNode(furnitureRef->Get3D())) {
			if (playerRef->actorState1.sitSleepState == RE::SIT_SLEEP_STATE::kIsSitting) {
				playerRef->InitiateGetUpPackage();
			}
		}
		else {
			RE::ClearFurniture(playerRef->currentProcess);
		}
	}

	void StaffCraftingMenu::InventoryUpdate(
		[[maybe_unused]] const RE::InventoryUpdateData* a_data,
		[[maybe_unused]] RE::UI_MESSAGE_RESULTS& a_result)
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

	void StaffCraftingMenu::PostDisplay()
	{
		const auto inventory3D = RE::Inventory3DManager::GetSingleton();
		assert(inventory3D);

		if (currentCategory != Category::Spell || craftItemPreview) {
			uiMovie->Display();
			inventory3D->Render();
		}
		else {
			inventory3D->Render();
			uiMovie->Display();
		}
	}

	RE::BSEventNotifyControl StaffCraftingMenu::ProcessEvent(
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
			uiMessageQueue->AddMessage(MENU_NAME, RE::UI_MESSAGE_TYPE::kHide, nullptr);
		}

		return kContinue;
	}

	void StaffCraftingMenu::ChooseItem(std::uint32_t a_index)
	{
		if (a_index >= listEntries.size()) {
			return;
		}

		const auto& entry = listEntries[a_index];
		RE::PlaySound(entry->selected ? "UISelectOff" : "UISelectOn");

		// TODO: see 51344
	}

	void StaffCraftingMenu::CategoryListEntry::SetupEntryObject(RE::GFxValue& a_entryObj) const
	{
		a_entryObj.SetMember("filterFlag", filterFlag);
		a_entryObj.SetMember("enabled", enabled);
		a_entryObj.SetMember("equipState", selected ? 1 : 0);
		SetupEntryObjectByType(a_entryObj);
	}

	void StaffCraftingMenu::ItemEntry::ShowInItemCard(StaffCraftingMenu* a_menu) const
	{
		a_menu->UpdateItemCard(data.get());
	}

	void StaffCraftingMenu::ItemEntry::ShowItem3D(bool a_show) const
	{
		const auto inventory3D = RE::Inventory3DManager::GetSingleton();
		assert(inventory3D);
		if (a_show) {
			inventory3D->UpdateItem3D(data.get());
		}
		else {
			inventory3D->Clear3D();
		}
	}

	const char* StaffCraftingMenu::ItemEntry::GetName() const
	{
		return data ? data->GetDisplayName() : "";
	}

	void StaffCraftingMenu::ItemEntry::SetupEntryObjectByType(RE::GFxValue& a_entryObj) const
	{
		a_entryObj.SetMember("text", GetName());
		a_entryObj.SetMember("count", data->countDelta);
	}

	void StaffCraftingMenu::SpellEntry::ShowInItemCard(StaffCraftingMenu* a_menu) const
	{
		a_menu->UpdateItemCard(data);
	}

	void StaffCraftingMenu::SpellEntry::ShowItem3D(bool a_show) const
	{
		const auto inventory3D = RE::Inventory3DManager::GetSingleton();
		assert(inventory3D);
		if (a_show && data) {
			inventory3D->UpdateMagic3D(const_cast<RE::SpellItem*>(data), 0);
		}
		else {
			inventory3D->Clear3D();
		}
	}

	const char* StaffCraftingMenu::SpellEntry::GetName() const
	{
		return data ? data->GetFullName() : "";
	}

	void StaffCraftingMenu::SpellEntry::SetupEntryObjectByType(RE::GFxValue& a_entryObj) const
	{
		a_entryObj.SetMember("text", GetName());
	}

	void StaffCraftingMenu::SetMenuDescription(const char* a_description)
	{
		RE::GFxValue menuDescription;
		if (menu.GetMember("MenuDescription", &menuDescription)) {
			menuDescription.SetMember("text", a_description);
		}
	}
}
