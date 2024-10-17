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
				labels{ "", "", "Staff", "Spell", "Morpholith" };

			const std::array<FilterFlag, Category::TOTAL> filters{
				FilterFlag::None,
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

	void StaffCraftingMenu::PopulateEntryList()
	{
		ClearEntryList();

		const auto playerRef = RE::PlayerCharacter::GetSingleton();
		assert(playerRef);

		const auto dataHandler = RE::TESDataHandler::GetSingleton();
		const auto idx_dragonrborn = dataHandler
			? dataHandler->GetModIndex("Dragonborn.esm"sv)
			: std::nullopt;
		const RE::FormID heartstoneID = idx_dragonrborn ? (*idx_dragonrborn << 24) | 0x17749 : 0x0;

		auto inventory = playerRef->GetInventory(
			[heartstoneID](RE::TESBoundObject& baseObj) -> bool
			{
				if (const auto weap = baseObj.As<RE::TESObjectWEAP>()) {
					return weap->IsStaff();
				}
				return baseObj.formID == heartstoneID;
			});

		for (auto& [baseObj, extra] : inventory) {
			auto& [count, entry] = extra;
			if (baseObj->IsWeapon()) {
				listEntries.push_back(RE::BSTSmartPointer(
					RE::make_smart<ItemEntry>(std::move(entry), FilterFlag::Staff)));
			}
			else if (const auto keywordForm = baseObj->As<RE::BGSKeywordForm>()) {
				if (baseObj->formID != heartstoneID)
					continue;

				listEntries.push_back(RE::BSTSmartPointer(
					RE::make_smart<ItemEntry>(std::move(entry), FilterFlag::Morpholith)));
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
		UpdateIngredients();
	}

	void StaffCraftingMenu::ClearEntryList()
	{
		listEntries.clear();
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

	void StaffCraftingMenu::UpdateEnabledEntries(std::uint32_t a_flags, bool a_fullRebuild)
	{
		(void)a_flags;
		// TODO: see 51450

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
								RE::UIMessageDataFactory::Create(
									interfaceStrings->bsUIMessageData));
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

		// TODO: see 51344
		if (entry->filterFlag != FilterFlag::Recipe) {
			if (CanSelectEntry(a_index, true)) {
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

	bool StaffCraftingMenu::CanSelectEntry(std::uint32_t a_index, bool a_showNotification)
	{
		(void)a_index;
		(void)a_showNotification;
		// TODO
		return true;
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
