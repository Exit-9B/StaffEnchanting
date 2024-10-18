#include "StaffCraftingMenu.h"

#include "RE/Misc.h"
#include "RE/Offset.h"

namespace UI
{
	StaffCraftingMenu::~StaffCraftingMenu()
	{
		ClearEffects();

		const auto inventory3D = RE::Inventory3DManager::GetSingleton();
		assert(inventory3D);
		inventory3D->unk159 = 1;
		inventory3D->unk158 = 1;
		UpdateItemPreview(nullptr);
	}

	void StaffCraftingMenu::ClearEffects()
	{
		const auto createdObjectManager = RE::BGSCreatedObjectManager::GetSingleton();
		assert(createdObjectManager);
		createdObjectManager->DestroyEnchantment(createdEnchantment, true);
		createdEnchantment = nullptr;
		createdEffects.clear();
	}

	void StaffCraftingMenu::Init()
	{
		// TODO: localization
		SetMenuDescription(
			"Staff Enchanting: Combine a Staff, Spell, and Morpholith to create magic staves");
		menu.SetMember("bCanCraft", true);
		menu.GetMember("CategoryList", &inventoryLists);
		if (inventoryLists.IsObject()) {
			const std::array<const char*, Category::TOTAL>
				labels{ "Special", "", "Staff", "Spell", "Morpholith" };

			static constexpr std::array<FilterFlag, Category::TOTAL> filters{
				FilterFlag::Recipe,
				FilterFlag::None,
				FilterFlag::Staff,
				FilterFlag::Spell,
				FilterFlag::Morpholith
			};

			enum
			{
				Text,
				Flag,
				DontHide,
				NumObjKeys
			};

			std::array<RE::GFxValue, Category::TOTAL * util::to_underlying(NumObjKeys)> categories;
			for (const auto i : std::views::iota(0ull, Category::TOTAL)) {
				categories[i * NumObjKeys + Text] = labels[i];
				categories[i * NumObjKeys + Flag] = filters[i];
				categories[i * NumObjKeys + DontHide] = true;
			}

			inventoryLists.Invoke("SetCategoriesList", categories);

			RE::GFxValue categoriesList;
			inventoryLists.GetMember("CategoriesList", &categoriesList);
			if (categoriesList.IsObject()) {
				categoriesList.GetMember("entryList", &categoryEntryList);
			}

			if (categoryEntryList.IsArray()) {
				RE::GFxValue divider;
				if (categoryEntryList.GetElement(Category::Divider, &divider)) {
					divider.SetMember("divider", true);
				}
				inventoryLists.Invoke("InvalidateListData");
			}

			PopulateEntryList();
		}

		UpdateInterface();
		UpdateBottomBar(RE::ActorValue::kEnchanting);
	}

	[[nodiscard]] static bool IsFavorite(const RE::InventoryEntryData* a_entry)
	{
		if (a_entry->extraLists) {
			for (const auto extraList : *a_entry->extraLists) {
				if (extraList->HasType<RE::ExtraHotkey>()) {
					return true;
				}
			}
		}
		return false;
	}

	[[nodiscard]] static bool IsSpecial(const RE::BGSConstructibleObject* a_obj)
	{
		for (auto condition = a_obj->conditions.head; condition; condition = condition->next) {
			if (condition &&
				condition->data.functionData.function ==
					RE::FUNCTION_DATA::FunctionID::kHasSpell) {
				return false;
			}
		}
		return true;
	}

	void StaffCraftingMenu::PopulateEntryList(bool a_fullRebuild)
	{
		listEntries.clear();
		ClearSelection();

		const auto playerRef = RE::PlayerCharacter::GetSingleton();
		assert(playerRef);

		const auto dataHandler = RE::TESDataHandler::GetSingleton();
		const auto idx_dragonrborn = dataHandler
			? dataHandler->GetModIndex("Dragonborn.esm"sv)
			: std::nullopt;
		const RE::FormID heartstoneID = idx_dragonrborn ? (*idx_dragonrborn << 24) | 0x17749 : 0x0;
		const auto defaultObjects = RE::BGSDefaultObjectManager::GetSingleton();
		const auto MagicDisallowEnchanting = defaultObjects->GetObject<RE::BGSKeyword>(
			RE::DEFAULT_OBJECT::kKeywordDisallowEnchanting);

		static const bool
			essentialFavorites = SKSE::WinAPI::GetModuleHandle("po3_EssentialFavorites") !=
			nullptr;

		const auto disallowHeartStonesKwd = dataHandler->LookupForm<RE::BGSKeyword>(
			0x800,
			"StaffEnchanting.esp");
		const auto allowSoulGemsKwd = dataHandler->LookupForm<RE::BGSKeyword>(
			0x801,
			"StaffEnchanting.esp");

		const bool disallowHeartStones = disallowHeartStonesKwd
			? workbench->HasKeyword(disallowHeartStonesKwd)
			: false;
		const bool allowSoulGems = allowSoulGemsKwd
			? workbench->HasKeyword(allowSoulGemsKwd)
			: false;

		auto inventory = playerRef->GetInventory(
			[heartstoneID](RE::TESBoundObject& baseObj) -> bool
			{
				if (const auto weap = baseObj.As<RE::TESObjectWEAP>()) {
					return weap->IsStaff();
				}
				return baseObj.formID == heartstoneID || baseObj.IsSoulGem();
			});

		for (auto& [baseObj, extra] : inventory) {
			auto& [count, entry] = extra;
			if (entry->IsQuestObject() || (essentialFavorites && IsFavorite(entry.get())))
				continue;

			if (baseObj->IsWeapon()) {
				if (entry->IsEnchanted() || !baseObj->As<RE::TESObjectWEAP>()->IsStaff()) {
					continue;
				}

				const auto entryKwdForm = baseObj->As<RE::BGSKeywordForm>();
				if (!entryKwdForm || entryKwdForm->HasKeyword(MagicDisallowEnchanting))
					continue;

				listEntries.push_back(RE::BSTSmartPointer(
					RE::make_smart<ItemEntry>(std::move(entry), FilterFlag::Staff)));
			}
			else if (allowSoulGems && baseObj->IsSoulGem()) {
				if (entry->GetSoulLevel() == RE::SOUL_LEVEL::kNone)
					continue;

				listEntries.push_back(RE::make_smart<ItemEntry>(
					std::move(entry), FilterFlag::Morpholith));
			}
			else if (!disallowHeartStones && baseObj->formID == heartstoneID) {
				listEntries.push_back(RE::make_smart<ItemEntry>(
					std::move(entry), FilterFlag::Morpholith));
			}
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

		const auto invChanges = playerRef->GetInventoryChanges();

		for (const auto obj : dataHandler->GetFormArray<RE::BGSConstructibleObject>()) {
			if (obj->CanBeCreatedOnWorkbench(workbench, true) && IsSpecial(obj)) {
				const auto& entry = listEntries.emplace_back(RE::make_smart<RecipeEntry>(obj));

				const auto& items = obj->requiredItems;
				for (const auto* const item :
					 std::span(items.containerObjects, items.numContainerObjects)) {
					if (item->count >
						invChanges->GetCount(item->obj, RE::InventoryUtils::QuestItemFilter)) {
						entry->enabled = false;
						break;
					}
				}
			}
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

		UpdateEnabledEntries(FilterFlag::All, a_fullRebuild);
		UpdateIngredients();
	}

	void StaffCraftingMenu::ClearSelection()
	{
		selected.Clear();
		UpdateItemPreview(nullptr);
		UpdateEnabledEntries();
		UpdateIngredients();
		UpdateItemList(listEntries, false);
		ClearEffects();
	}

	void StaffCraftingMenu::UpdateItemPreview(std::unique_ptr<RE::InventoryEntryData>&& a_item)
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

	void StaffCraftingMenu::UpdateEnabledEntries(FilterFlag a_flags, bool a_fullRebuild)
	{
		const SKSE::stl::enumeration flags{ a_flags };

		for (const auto& entry : listEntries) {
			entry->enabled = flags.all(entry->filterFlag) && CanSelectEntry(entry, false);
		}

		if (menu.IsObject()) {
			menu.Invoke("UpdateItemList", std::to_array<RE::GFxValue>({ a_fullRebuild }));
		}
	}

	void StaffCraftingMenu::UpdateEnchantment()
	{
		if (craftItemPreview) {
			if (craftItemPreview->extraLists && !craftItemPreview->extraLists->empty()) {
				const auto& extraList = craftItemPreview->extraLists->front();
				extraList->SetEnchantment(nullptr, 0, false);
			}
		}

		ClearEffects();

		if (selected.spell) {
			// TODO: calculate correct charge amount
			chargeAmount = static_cast<float>(*"iSoulLevelValueGrand"_gs);

			for (const auto& effect : selected.spell->data->effects) {
				auto& createdEffect = createdEffects.emplace_back();
				createdEffect.Copy(effect);
				// TODO: calculate magnitude and duration changes
			}

			if (!createdEffects.empty()) {
				const auto createdObjectManager = RE::BGSCreatedObjectManager::GetSingleton();
				assert(createdObjectManager);
				createdEnchantment = createdObjectManager->AddWeaponEnchantment(createdEffects);
			}
		}

		if (craftItemPreview && createdEnchantment) {
			if (craftItemPreview->extraLists && !craftItemPreview->extraLists->empty()) {
				const auto& extraList = craftItemPreview->extraLists->front();
				extraList->SetEnchantment(
					createdEnchantment,
					static_cast<std::uint16_t>(chargeAmount),
					false);
			}
			else {
				// InventoryEntryData does not take ownership, so we need to hold ownership.
				tempExtraList = std::make_unique<RE::ExtraDataList>();
				tempExtraList->SetEnchantment(
					createdEnchantment,
					static_cast<std::uint16_t>(chargeAmount),
					false);
				craftItemPreview->AddExtraList(tempExtraList.get());
			}

			UpdateItemPreview(std::move(craftItemPreview));
		}
	}

	void StaffCraftingMenu::UpdateIngredients()
	{
		RE::GFxValue ingredients;
		uiMovie->CreateArray(&ingredients);

		if (currentCategory != Category::Recipe || highlightIndex >= listEntries.size() ||
			listEntries[highlightIndex]->filterFlag != FilterFlag::Recipe) {

			RE::GFxValue staff;
			uiMovie->CreateObject(&staff);
			staff.SetMember("Name", selected.staff ? selected.staff->GetName() : "Staff");
			staff.SetMember("RequiredCount", 1);
			staff.SetMember("PlayerCount", selected.staff ? 1 : 0);
			ingredients.PushBack(staff);

			RE::GFxValue spell;
			uiMovie->CreateObject(&spell);
			spell.SetMember("Name", selected.spell ? selected.spell->GetName() : "Spell");
			spell.SetMember("RequiredCount", 1);
			spell.SetMember("PlayerCount", selected.spell ? 1 : 0);
			ingredients.PushBack(spell);

			RE::GFxValue morpholith;
			uiMovie->CreateObject(&morpholith);
			morpholith.SetMember(
				"Name",
				selected.morpholith ? selected.morpholith->GetName() : "Morpholith");
			morpholith.SetMember("RequiredCount", 1);
			morpholith.SetMember("PlayerCount", selected.morpholith ? 1 : 0);
			ingredients.PushBack(morpholith);
		}
		else {
			const auto playerRef = RE::PlayerCharacter::GetSingleton();
			const auto invChanges = playerRef->GetInventoryChanges();

			const auto entry = static_cast<const RecipeEntry*>(listEntries[highlightIndex].get());
			assert(entry->data);
			const auto& items = entry->data->requiredItems;
			for (const auto* const item :
				 std::span(items.containerObjects, items.numContainerObjects)) {
				RE::GFxValue ingredient;
				uiMovie->CreateObject(&ingredient);
				ingredient.SetMember("Name", item->obj->GetName());
				ingredient.SetMember("RequiredCount", item->count);
				ingredient.SetMember(
					"PlayerCount",
					invChanges->GetCount(item->obj, RE::InventoryUtils::QuestItemFilter));
				ingredients.PushBack(ingredient);
			}
		}

		menu.Invoke(
			"UpdateIngredients",
			std::to_array<RE::GFxValue>({ *"sRequirementsText"_gs, ingredients, false }));
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

		if (currentCategory == Category::Recipe) {
			UpdateIngredients();
		}
	}

	bool StaffCraftingMenu::ProcessUserEvent(const RE::BSFixedString& a_userEvent)
	{
		// TBD: magnitude slider

		const auto userEvents = RE::UserEvents::GetSingleton();
		assert(userEvents);
		if (a_userEvent == userEvents->cancel) {
			// TODO: skip confirmation if nothing selected
			if (exiting) {
				return false;
			}

			const auto msgBoxData = new RE::MessageBoxData();
			msgBoxData->bodyText = *"sQuitEnchanting"_gs;
			msgBoxData->buttonText.push_back(*"sYes"_gs);
			msgBoxData->buttonText.push_back(*"sNo"_gs);

			const auto callback = RE::MakeMessageBoxCallback(
				[this](auto msg)
				{
					if (msg != RE::IMessageBoxCallback::Message::kUnk0) {
						exiting = false;
					}
					else {
						const auto uiMessageQueue = RE::UIMessageQueue::GetSingleton();
						if (uiMessageQueue) {
							const auto interfaceStrings = RE::InterfaceStrings::GetSingleton();
							const auto userEvents = RE::UserEvents::GetSingleton();
							assert(interfaceStrings);
							assert(userEvents);

							const auto msgData = static_cast<RE::BSUIMessageData*>(
								RE::UIMessageDataFactory::Create(
									interfaceStrings->bsUIMessageData));
							assert(msgData);
							msgData->fixedStr = userEvents->cancel;

							uiMessageQueue
								->AddMessage(MENU_NAME, RE::UI_MESSAGE_TYPE::kUserEvent, msgData);
						}
					}
				});

			msgBoxData->callback = callback;
			msgBoxData->unk38 = 25;
			msgBoxData->unk48 = 4;

			msgBoxData->QueueMessage();
			exiting = true;
			return true;
		}
		else if (a_userEvent == userEvents->xButton) {
			// TODO: craft
			return true;
		}
		else if (a_userEvent == userEvents->yButton) {
			// TODO: edit name
			return true;
		}

		return false;
	}

	bool StaffCraftingMenu::RenderItem3DOnTop() const
	{
		return currentCategory != Category::Spell || craftItemPreview;
	}

	void StaffCraftingMenu::ChooseItem(std::uint32_t a_index)
	{
		if (a_index >= listEntries.size()) {
			return;
		}

		const auto& entry = listEntries[a_index];
		RE::PlaySound(entry->selected ? "UISelectOff" : "UISelectOn");

		if (entry->filterFlag == FilterFlag::Recipe) {
			if (entry->enabled) {
				ClearSelection();
				entry->selected = true;
				UpdateItemList(listEntries);
				const auto msgBoxData = new RE::MessageBoxData();
				msgBoxData->bodyText = *"sConstructibleMenuConfirm"_gs;
				msgBoxData->buttonText.push_back(*"sYes"_gs);
				msgBoxData->buttonText.push_back(*"sNo"_gs);

				const auto callback = RE::MakeMessageBoxCallback(
					[&](auto message)
					{
						entry->selected = false;
						if (message != RE::IMessageBoxCallback::Message::kUnk0) {
							UpdateItemList(listEntries);
						}
						else {
							CreateItem(static_cast<RecipeEntry*>(entry.get())->data);
						}
					});

				msgBoxData->callback = callback;
				msgBoxData->unk38 = 25;
				msgBoxData->unk48 = 4;

				msgBoxData->QueueMessage();
			}
			else {
				RE::SendHUDMessage::ShowHUDMessage(*"sLackRequiredToCreate"_gs);
				RE::UIUtils::PlayMenuSound(RE::GetNoActivationSound());
			}
		}
		else {
			if (CanSelectEntry(entry, true)) {
				selected.Toggle(entry);

				std::unique_ptr<RE::InventoryEntryData> itemPreview;
				if (selected.staff) {
					// Vanilla EnchantConstructMenu would do a deep copy here, duplicating the
					// extra lists. To avoid memory leaks, we prefer to use a shallow copy.
					itemPreview = std::make_unique<RE::InventoryEntryData>(*selected.staff->data);
					itemPreview->countDelta = 1;
					if (itemPreview->IsWorn()) {
						// Do not delete removed lists, since we do not have ownership.
						itemPreview->SetWorn(false, true, false);
						itemPreview->SetWorn(false, false, false);
					}
				}
				UpdateItemPreview(std::move(itemPreview));
				UpdateEnchantment();
				// TODO: display error if insufficient charge

				UpdateInterface();
				UpdateEnabledEntries();
				UpdateItemList(listEntries, false);
				UpdateIngredients();
				// TBD: slider
			}
		}
	}

	bool StaffCraftingMenu::CanSelectEntry(
		const RE::BSTSmartPointer<CategoryListEntry>& a_entry,
		bool a_showNotification)
	{
		if (a_entry->filterFlag == FilterFlag::Recipe) {
			const auto playerRef = RE::PlayerCharacter::GetSingleton();
			const auto invChanges = playerRef->GetInventoryChanges();

			const auto recipe = static_cast<const RecipeEntry*>(a_entry.get());
			assert(recipe->data);
			const auto& items = recipe->data->requiredItems;
			for (const auto* const item :
				 std::span(items.containerObjects, items.numContainerObjects)) {
				if (item->count >
					invChanges->GetCount(item->obj, RE::InventoryUtils::QuestItemFilter)) {
					return false;
				}
			}
		}

		// TODO
		(void)a_showNotification;
		return true;
	}

	void StaffCraftingMenu::CreateItem(const RE::BGSConstructibleObject* a_constructible)
	{
		const auto createdItem = a_constructible->createdItem;
		const auto createdCount = a_constructible->data.numConstructed;

		const auto playerRef = RE::PlayerCharacter::GetSingleton();

		if (!a_constructible->CreateItem()) {
			return;
		}

		RE::SendHUDMessage::ShowInventoryChangeMessage(
			static_cast<RE::TESBoundObject*>(createdItem),
			createdCount,
			true,
			true);

		const auto skill = workbench->workBenchData.usesSkill;
		if (skill.underlying() - 6u <= 17u) {
			playerRef->UseSkill(skill.get(), a_constructible->CalcSkillUse());
		}

		const auto workbenchRef = playerRef->currentProcess->GetOccupiedFurniture().get();
		const auto storyEvent = RE::BGSCraftItemEvent(
			workbenchRef.get(),
			workbenchRef->GetCurrentLocation(),
			createdItem);
		const auto storyEventManager = RE::BGSStoryEventManager::GetSingleton();
		storyEventManager->AddEvent(storyEvent);

		const auto itemCraftedEvent = RE::ItemCrafted::Event{
			.item = createdItem,
			.unk08 = true,
			.unk09 = false,
			.unk0A = false,
		};
		const auto itemCraftedSource = RE::ItemCrafted::GetEventSource();
		itemCraftedSource->SendEvent(std::addressof(itemCraftedEvent));

		PopulateEntryList(true);
		UpdateItemPreview(nullptr);
		menu.Invoke("UpdateItemDisplay");

		RE::PlaySound("UISmithingCreateGeneric");
	}

	void StaffCraftingMenu::Selection::Clear()
	{
		if (staff) {
			staff->selected = false;
		}

		staff.reset();

		if (spell) {
			spell->selected = false;
		}

		spell.reset();
	}

	void StaffCraftingMenu::Selection::Toggle(
		const RE::BSTSmartPointer<CategoryListEntry>& a_entry)
	{
		if (a_entry->filterFlag == FilterFlag::Spell) {
			const auto spellEntry = static_cast<SpellEntry*>(a_entry.get());
			if (spell.get() == spellEntry) {
				spell = nullptr;
				a_entry->selected = false;
			}
			else {
				if (spell) {
					spell->selected = false;
				}
				spell.reset(spellEntry);
				a_entry->selected = true;
			}
		}
		else if (a_entry->filterFlag == FilterFlag::Staff) {
			const auto itemEntry = static_cast<ItemEntry*>(a_entry.get());
			if (staff.get() == itemEntry) {
				staff = nullptr;
				a_entry->selected = false;
			}
			else {
				if (staff) {
					staff->selected = false;
				}
				staff.reset(itemEntry);
				a_entry->selected = true;
			}
		}
		else if (a_entry->filterFlag == FilterFlag::Morpholith) {
			const auto itemEntry = static_cast<ItemEntry*>(a_entry.get());
			if (morpholith.get() == itemEntry) {
				morpholith = nullptr;
				a_entry->selected = false;
			}
			else {
				if (morpholith) {
					morpholith->selected = false;
				}
				morpholith.reset(itemEntry);
				a_entry->selected = true;
			}
		}
	}
}
