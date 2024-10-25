#include "StaffCraftingMenu.h"

#include "RE/Misc.h"
#include "RE/Offset.h"
#include "common/Forms.h"

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
		const char* sStaffEnchanting;
		RE::BSString sStaffEnchantMenuDescription;
		if (const auto MenuDescription = Forms::StaffEnchanting::MenuDescription()) {
			sStaffEnchanting = MenuDescription->GetName();
			MenuDescription->GetDescription(sStaffEnchantMenuDescription, nullptr);
		}
		else {
			sStaffEnchanting = "Staff Enchanting";
			sStaffEnchantMenuDescription =
				"Combine a Staff, Spell, and Morpholith to create magic staves"sv;
		}

		SetMenuDescription(
			fmt::format("{}: {}"sv, sStaffEnchanting, sStaffEnchantMenuDescription.c_str())
				.c_str());

		menu.SetMember("bCanCraft", true);
		menu.GetMember("CategoryList", &inventoryLists);
		if (inventoryLists.IsObject()) {
			enum
			{
				Text,
				Flag,
				DontHide,
				NumObjKeys
			};

			std::array<RE::GFxValue, Category::TOTAL * util::to_underlying(NumObjKeys)> categories;
			for (const auto i : std::views::iota(0ull, Category::TOTAL)) {
				categories[i * NumObjKeys + Text] = labels[i].c_str();
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
		heartStoneCount = 0;
		listEntries.clear();
		ClearSelection();

		const auto playerRef = RE::PlayerCharacter::GetSingleton();
		assert(playerRef);

		const auto dataHandler = RE::TESDataHandler::GetSingleton();
		const auto DLC2HeartStone = dataHandler->LookupForm(0x17749, "Dragonborn.esm"sv);

		const auto defaultObjects = RE::BGSDefaultObjectManager::GetSingleton();
		const auto MagicDisallowEnchanting = defaultObjects->GetObject<RE::BGSKeyword>(
			RE::DEFAULT_OBJECT::kKeywordDisallowEnchanting);

		static const bool
			essentialFavorites = SKSE::WinAPI::GetModuleHandle("po3_EssentialFavorites") !=
			nullptr;

		const auto disallowHeartStonesKwd = Forms::StaffEnchanting::DisallowHeartStones();
		const auto allowSoulGemsKwd = Forms::StaffEnchanting::AllowSoulGems();

		const bool disallowHeartStones = disallowHeartStonesKwd
			? workbench->HasKeyword(disallowHeartStonesKwd)
			: false;
		const bool allowSoulGems = allowSoulGemsKwd
			? workbench->HasKeyword(allowSoulGemsKwd)
			: false;

		const auto itemCount = RE::GetInventoryItemCount(playerRef);
		for (const auto i : std::views::iota(0, itemCount)) {
			std::unique_ptr<RE::InventoryEntryData> item{ RE::GetInventoryItemAt(playerRef, i) };
			if (!item) {
				continue;
			}

			const auto object = item->GetObject();
			if (!object || !object->GetName() || !object->GetPlayable()) {
				continue;
			}

			if (item->IsQuestObject() || item->IsEnchanted() ||
				(essentialFavorites && IsFavorite(item.get()))) {
				continue;
			}

			auto filterFlag = FilterFlag::None;
			if (!disallowHeartStones && object == DLC2HeartStone) {
				++heartStoneCount;
				filterFlag = FilterFlag::Morpholith;
			}
			else if (allowSoulGems && object->Is(RE::FormType::SoulGem)) {
				filterFlag = FilterFlag::Morpholith;
			}
			else if (const auto weap = object->As<RE::TESObjectWEAP>()) {
				if (weap->IsStaff() && !weap->formEnchanting &&
					!weap->HasKeyword(MagicDisallowEnchanting)) {

					filterFlag = FilterFlag::Staff;
				}
			}

			if (filterFlag != FilterFlag::None) {
				listEntries.emplace_back(RE::make_smart<ItemEntry>(std::move(item), filterFlag));
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

	float StaffCraftingMenu::GetEntryDataSoulCharge(RE::InventoryEntryData* a_entry)
	{
		switch (a_entry->GetSoulLevel()) {
		case RE::SOUL_LEVEL::kPetty:
			return static_cast<float>(*"iSoulLevelValuePetty"_gs);
		case RE::SOUL_LEVEL::kLesser:
			return static_cast<float>(*"iSoulLevelValuelesser"_gs);
		case RE::SOUL_LEVEL::kCommon:
			return static_cast<float>(*"iSoulLevelValueCommon"_gs);
		case RE::SOUL_LEVEL::kGreater:
			return static_cast<float>(*"iSoulLevelValueGreater"_gs);
		case RE::SOUL_LEVEL::kGrand:
			return static_cast<float>(*"iSoulLevelValueGrand"_gs);
		}
		return -1.0f;
	}

	int32_t StaffCraftingMenu::GetSpellLevel(const RE::SpellItem* a_spell)
	{
		int32_t response = 0;
		for (const auto effect : a_spell->effects) {
			if (!effect || !effect->baseEffect)
				continue;

			response = effect->baseEffect->data.minimumSkill > response
				? effect->baseEffect->data.minimumSkill
				: response;
		}
		return response;
	}

	bool StaffCraftingMenu::MagicEffectHasDescription(RE::EffectSetting* a_effect)
	{
		assert(a_effect);
		if (a_effect->data.flags.any(RE::EffectSetting::EffectSettingData::Flag::kHideInUI)) {
			return false;
		}

		return !a_effect->magicItemDescription.empty();
	}

	bool StaffCraftingMenu::CanCraftWithSpell(const RE::SpellItem* a_spell)
	{
		const auto minLevel = GetSpellLevel(a_spell);
		if (minLevel < 25) {
			return heartStoneCount > 0;
		}
		else if (minLevel < 50) {
			return heartStoneCount > 1;
		}
		else if (minLevel < 75) {
			return heartStoneCount > 2;
		}
		else if (minLevel < 90) {
			return heartStoneCount > 3;
		}
		else {
			return heartStoneCount > 4;
		}
	}

	void StaffCraftingMenu::UpdateEnchantmentCharge()
	{
		if (selected.morpholith) {
			float response = GetEntryDataSoulCharge(selected.morpholith->data.get());
			if (response < 0.0f) {
				chargeAmount = static_cast<float>(*"iSoulLevelValueGrand"_gs);
			}
			else {
				chargeAmount = response;
			}
		}
		else {
			for (const auto& entry : listEntries) {
				if (entry->filterFlag != FilterFlag::Morpholith)
					continue;
				const auto itemEntry = static_cast<const ItemEntry*>(entry.get());
				if (!itemEntry)
					continue;
				const auto entryData = itemEntry->data.get();
				if (!entryData)
					continue;

				float response = GetEntryDataSoulCharge(entryData);

				if (response < 0.0f) {
					chargeAmount = static_cast<float>(*"iSoulLevelValueGrand"_gs);
				}
				if (response > chargeAmount) {
					chargeAmount = response;
				}
			}
		}
	}

	[[nodiscard]] static std::string MakeSuggestedName(const RE::SpellItem* a_spell)
	{
		const auto spellName = a_spell->GetName();
		const auto CreatedStaffName = Forms::StaffEnchanting::CreatedStaffName();
		const auto format = CreatedStaffName ? CreatedStaffName->GetName() : "Staff of %s";
		const int size = std::snprintf(nullptr, 0, format, spellName);
		if (size < 0) {
			return std::string(format);
		}

		std::string suggestedName = std::string(size, '\0');
		std::snprintf(suggestedName.data(), suggestedName.size() + 1, format, spellName);
		return suggestedName;
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
			UpdateEnchantmentCharge();

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
			suggestedName = MakeSuggestedName(selected.spell->data);

			if (craftItemPreview->extraLists && !craftItemPreview->extraLists->empty()) {
				const auto& extraList = craftItemPreview->extraLists->front();
				extraList->SetEnchantment(
					createdEnchantment,
					static_cast<std::uint16_t>(chargeAmount),
					false);

				RE::SetOverrideName(extraList, suggestedName.c_str());
			}
			else {
				// InventoryEntryData does not take ownership, so we need to hold ownership.
				tempExtraList = std::make_unique<RE::ExtraDataList>();
				tempExtraList->SetEnchantment(
					createdEnchantment,
					static_cast<std::uint16_t>(chargeAmount),
					false);
				craftItemPreview->AddExtraList(tempExtraList.get());

				RE::SetOverrideName(tempExtraList.get(), suggestedName.c_str());
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
			staff.SetMember(
				"Name",
				selected.staff ? selected.staff->GetName() : labels[Category::Staff].c_str());
			staff.SetMember("RequiredCount", 1);
			staff.SetMember("PlayerCount", selected.staff ? 1 : 0);
			ingredients.PushBack(staff);

			RE::GFxValue spell;
			uiMovie->CreateObject(&spell);
			spell.SetMember(
				"Name",
				selected.spell ? selected.spell->GetName() : labels[Category::Spell].c_str());
			spell.SetMember("RequiredCount", 1);
			spell.SetMember("PlayerCount", selected.spell ? 1 : 0);
			ingredients.PushBack(spell);

			RE::GFxValue morpholith;
			uiMovie->CreateObject(&morpholith);
			morpholith.SetMember(
				"Name",
				selected.morpholith
					? selected.morpholith->GetName()
					: labels[Category::Morpholith].c_str());
			if (selected.spell) {
				const auto spellLevel = GetSpellLevel(selected.spell->data);

				if (spellLevel < 25) {
					morpholith.SetMember("RequiredCount", 1);
				}
				else if (spellLevel < 50) {
					morpholith.SetMember("RequiredCount", 2);
				}
				else if (spellLevel < 75) {
					morpholith.SetMember("RequiredCount", 3);
				}
				else if (spellLevel < 90) {
					morpholith.SetMember("RequiredCount", 4);
				}
				else {
					morpholith.SetMember("RequiredCount", 5);
				}
			}
			else {
				morpholith.SetMember("RequiredCount", 1);
			}

			morpholith.SetMember("PlayerCount", selected.morpholith ? heartStoneCount : 0);
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

	bool StaffCraftingMenu::IsSpellValid(const RE::SpellItem* a_spell)
	{
		if (a_spell->CalculateMagickaCost(nullptr) < 1.0f) {
			return false;
		}
		if (a_spell->effects.empty()) {
			return false;
		}
		if (a_spell->GetDelivery() == RE::MagicSystem::Delivery::kSelf) {
			return false;
		}

		static constexpr RE::FormID ritualEffectID = 0x806E1;
		static constexpr RE::FormID ritualEffectIllusionID = 0x8BB92;

		const auto defaultObjects = RE::BGSDefaultObjectManager::GetSingleton();
		const auto eitherHandForm = defaultObjects->GetObject<RE::BGSEquipSlot>(
			RE::DEFAULT_OBJECT::kEitherHandEquip);
		if (!eitherHandForm) {
			return false;
		}

		if (const auto spellEquipSlot = a_spell->GetEquipSlot()) {
			if (spellEquipSlot != eitherHandForm) {
				return false;
			}
		}
		else {
			return false;
		}

		bool hasDescription = false;
		for (const auto& effect : a_spell->effects) {
			if (!effect->baseEffect) {
				return false;
			}

			const auto effectKwdForm = effect->baseEffect->As<RE::BGSKeywordForm>();
			if (!effectKwdForm || effectKwdForm->HasKeyword(ritualEffectID) ||
				effectKwdForm->HasKeyword(ritualEffectIllusionID)) {
				return false;
			}

			if (!hasDescription && MagicEffectHasDescription(effect->baseEffect)) {
				hasDescription = true;
			}
		}
		return hasDescription;
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

		if (!IsSpellValid(a_spell))
			return;

		const auto smartEntry = RE::make_smart<SpellEntry>(a_spell);
		smartEntry->enabled = CanCraftWithSpell(a_spell);
		a_entries.push_back(smartEntry);
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
			if (exiting || selected.Empty()) {
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
							const auto userEvents = RE::UserEvents::GetSingleton();
							assert(userEvents);

							const auto
								msgData = RE::UIMessageDataFactory::Create<RE::BSUIMessageData>();
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
			if (!selected.staff || !selected.spell || !selected.morpholith) {
				// TODO: Multi-morpholith check here for higher level spells.
				return true;
			}

			const auto msgBoxData = new RE::MessageBoxData();
			msgBoxData->bodyText = *"sEnchantItem"_gs;
			msgBoxData->buttonText.push_back(*"sYes"_gs);
			msgBoxData->buttonText.push_back(*"sNo"_gs);

			const auto callback = RE::MakeMessageBoxCallback(
				[&](auto message)
				{
					if (message == RE::IMessageBoxCallback::Message::kUnk0) {
						CreateStaff();
					}
				});

			msgBoxData->callback = callback;
			msgBoxData->unk38 = 25;
			msgBoxData->unk48 = 4;

			msgBoxData->QueueMessage();
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
		else if (a_entry->filterFlag == FilterFlag::Spell) {
			const auto spellEntry = static_cast<const SpellEntry*>(a_entry.get());
			assert(spellEntry && spellEntry->data);

			return CanCraftWithSpell(spellEntry->data);
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

	void StaffCraftingMenu::CreateStaff()
	{
		if (!selected.staff || !selected.spell || !selected.morpholith || !craftItemPreview) {
			return;
		}
		const auto player = RE::PlayerCharacter::GetSingleton();
		const auto staff = selected.staff->data->GetObject();
		const auto enchantment = RE::BGSCreatedObjectManager::GetSingleton()->AddWeaponEnchantment(
			createdEffects);
		if (!player || !staff || !enchantment)
			return;

		RE::ExtraDataList* const createdExtraList = RE::EnchantObject(
			player->GetInventoryChanges(),
			staff,
			nullptr,
			enchantment,
			static_cast<uint16_t>(chargeAmount));
		if (!createdExtraList)
			return;

		const char* newName = "";
		if (customName.empty()) {
			newName = suggestedName.c_str();
		}
		else {
			newName = customName.c_str();
		}
		RE::SetOverrideName(createdExtraList, newName);

		player->RemoveItem(
			selected.morpholith->data->GetObject(),
			1,
			RE::ITEM_REMOVE_REASON::kRemove,
			nullptr,
			nullptr);

		RE::SendHUDMessage::ShowInventoryChangeMessage(staff, 1, true, true, newName);

		const auto skill = workbench->workBenchData.usesSkill;
		if (skill.underlying() - 6u <= 17u) {
			player->UseSkill(skill.get(), 20.0f);
			UpdateBottomBar(skill.get());
		}

		const auto workbenchRef = player->currentProcess->GetOccupiedFurniture().get();
		const auto storyEvent = RE::BGSCraftItemEvent(
			workbenchRef.get(),
			workbenchRef->GetCurrentLocation(),
			enchantment);
		const auto storyEventManager = RE::BGSStoryEventManager::GetSingleton();
		storyEventManager->AddEvent(storyEvent);

		const auto itemCraftedEvent = RE::ItemCrafted::Event{
			.item = staff,
			.unk08 = false,
			.unk09 = true,
			.unk0A = false,
		};
		const auto itemCraftedSource = RE::ItemCrafted::GetEventSource();
		itemCraftedSource->SendEvent(std::addressof(itemCraftedEvent));

		PopulateEntryList(true);
		UpdateItemPreview(nullptr);
		menu.Invoke("UpdateItemDisplay");
		RE::PlaySound("UIEnchantingItemCreate");
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
