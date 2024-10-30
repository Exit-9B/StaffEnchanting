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

		UpdateTextElements();
		const auto skill = workbench->workBenchData.usesSkill;
		if (skill >= RE::ActorValue::kOneHanded && skill <= RE::ActorValue::kEnchanting) {
			UpdateBottomBar(skill.get());
		}

		const auto HelpStaffEnchantingShort = Forms::StaffEnchanting::HelpStaffEnchantingShort();
		RE::TutorialMenu::OpenTutorialMenu(HelpStaffEnchantingShort);
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
		highestMorpholithCount = 0;
		maxSoulSize = 0.0f;
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

		std::vector<const RE::BGSKeyword*> acceptedMorpholiths{};
		for (const auto keyword : std::span(workbench->keywords, workbench->numKeywords)) {
			if (!keyword)
				continue;

			const auto str = std::string_view(keyword->formEditorID);
			static constexpr auto prefix = "EnchantMorpholith"sv;
			if (str.size() <= prefix.size() ||
				::_strnicmp(str.data(), prefix.data(), prefix.size()) != 0) {
				continue;
			}

			acceptedMorpholiths.push_back(keyword);
		}

		const auto itemCount = playerRef->GetInventoryItemCount();
		for (const auto i : std::views::iota(0, itemCount)) {
			std::unique_ptr<RE::InventoryEntryData> item{ playerRef->GetInventoryItemAt(i) };
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
				highestMorpholithCount = std::max(highestMorpholithCount, item->countDelta);
				filterFlag = FilterFlag::Morpholith;
			}
			else if (allowSoulGems && object->Is(RE::FormType::SoulGem)) {
				const auto currentCharge = GetEntryDataSoulCharge(item.get());
				if (currentCharge > 0.0f) {
					maxSoulSize = std::max(currentCharge, maxSoulSize);
					filterFlag = FilterFlag::Morpholith;
				}
			}
			else if (const auto weap = object->As<RE::TESObjectWEAP>(); weap && weap->IsStaff()) {
				if (!weap->formEnchanting && !weap->HasKeyword(MagicDisallowEnchanting)) {

					filterFlag = FilterFlag::Staff;
				}
			}
			else if (const auto obj = object->As<RE::BGSKeywordForm>();
					 obj && IsValidMorpholith(obj, acceptedMorpholiths)) {
				highestMorpholithCount = std::max(highestMorpholithCount, item->countDelta);
				filterFlag = FilterFlag::Morpholith;
			}

			if (filterFlag != FilterFlag::None) {
				listEntries.emplace_back(RE::make_smart<ItemEntry>(std::move(item), filterFlag));
			}
		}

		const auto playerBase = playerRef->GetActorBase();
		assert(playerBase);
		const auto spellData = playerBase->actorEffects;
		assert(spellData);

		const auto spellLists = {
			std::span(spellData->spells, spellData->numSpells),
			std::span(playerRef->addedSpells)
		};

		for (const auto spell : std::views::join(spellLists)) {
			if (IsSpellValid(spell)) {
				const auto& entry = listEntries.emplace_back(RE::make_smart<SpellEntry>(spell));
				entry->enabled = CanCraftWithSpell(spell);
			}
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
		UpdateTextElements();
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

	bool StaffCraftingMenu::CanSetOverrideName(RE::InventoryEntryData* a_item)
	{
		const auto extraLists = a_item ? a_item->extraLists : nullptr;
		if (extraLists && !extraLists->empty()) {
			const auto extraList = extraLists->front();
			const auto extraTextData = extraList ? extraList->GetExtraTextDisplayData() : nullptr;
			if (extraTextData && (extraTextData->displayNameText || extraTextData->ownerQuest)) {
				return false;
			}
		}

		return true;
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

	std::int32_t StaffCraftingMenu::GetSpellHeartstones(const RE::SpellItem* a_spell)
	{
		return GetSpellLevel(a_spell) + 1;
	}

	float StaffCraftingMenu::GetDefaultCharge(const RE::SpellItem* a_spell)
	{
		return std::max(500.0f, util::to_underlying(GetSpellLevel(a_spell)) * 1000.0f);
	}

	bool StaffCraftingMenu::MagicEffectHasDescription(RE::EffectSetting* a_effect)
	{
		assert(a_effect);
		if (a_effect->data.flags.any(RE::EffectSetting::EffectSettingData::Flag::kHideInUI)) {
			return false;
		}

		return !a_effect->magicItemDescription.empty();
	}

	bool StaffCraftingMenu::CanCraftWithSpell(const RE::SpellItem* a_spell) const
	{
		if (selected.morpholith) {
			const auto morpholithCharge = GetEntryDataSoulCharge(selected.morpholith->data.get());
			const auto morpholithCount = selected.morpholith->data->countDelta;
			return morpholithCharge > 0.0f
				? morpholithCharge >= CalculateSpellCost(a_spell)
				: morpholithCount >= GetSpellHeartstones(a_spell);
		}
		return highestMorpholithCount >= GetSpellHeartstones(a_spell) ||
			maxSoulSize >= CalculateSpellCost(a_spell);
	}

	bool StaffCraftingMenu::IsValidMorpholith(
		const RE::BGSKeywordForm* a_obj,
		const std::vector<const RE::BGSKeyword*>& a_vec)
	{
		if (a_vec.empty()) {
			return false;
		}

		for (const auto recordedKeyword : a_vec) {
			if (a_obj->HasKeyword(recordedKeyword)) {
				return true;
			}
		}
		return false;
	}

	void StaffCraftingMenu::UpdateEnchantmentCharge()
	{
		if (selected.morpholith) {
			const auto& entryData = selected.morpholith->data;
			const auto soulValue = GetEntryDataSoulCharge(entryData.get());
			chargeAmount = soulValue > 0.0f ? soulValue : GetDefaultCharge(selected.spell->data);
		}
		else {
			float maxCharge = std::numeric_limits<float>::lowest();
			for (const auto& entry : listEntries) {
				if (!entry || entry->filterFlag != FilterFlag::Morpholith)
					continue;
				const auto itemEntry = static_cast<const ItemEntry*>(entry.get());
				const auto& entryData = itemEntry->data;
				if (!entryData)
					continue;
				const auto soulValue = GetEntryDataSoulCharge(entryData.get());
				const auto itemCharge = soulValue > 0.0f
					? soulValue
					: GetDefaultCharge(selected.spell->data);
				maxCharge = std::max(maxCharge, itemCharge);
			}

			chargeAmount = maxCharge > 0.0f ? maxCharge : GetDefaultCharge(selected.spell->data);
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
			UpdateEnchantmentCharge();
			for (const auto& effect : selected.spell->data->effects) {
				auto& createdEffect = createdEffects.emplace_back();
				createdEffect.Copy(effect);
				if (createdEffect.baseEffect->GetArchetype() ==
					RE::EffectSetting::Archetype::kLight) {
					createdEffect.SetMagnitude(1.0f);
				}
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

				extraList->SetOverrideName(suggestedName.c_str());
			}
			else {
				const auto extraList = new RE::ExtraDataList();
				extraList->SetEnchantment(
					createdEnchantment,
					static_cast<std::uint16_t>(chargeAmount),
					false);

				extraList->SetOverrideName(suggestedName.c_str());
				craftItemPreview->AddExtraList(extraList);
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
			if (!selected.spell || !selected.morpholith ||
				selected.morpholith->data->GetObject()->IsSoulGem()) {
				morpholith.SetMember("RequiredCount", 1);
				morpholith.SetMember("PlayerCount", selected.morpholith ? 1 : 0);
			}
			else {
				const auto playerRef = RE::PlayerCharacter::GetSingleton();
				const auto invChanges = playerRef->GetInventoryChanges();
				morpholith.SetMember("RequiredCount", GetSpellHeartstones(selected.spell->data));
				morpholith.SetMember(
					"PlayerCount",
					invChanges->GetCount(
						selected.morpholith->data->GetObject(),
						RE::InventoryUtils::QuestItemFilter));
			}
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
		const auto castingType = a_spell->GetCastingType();
		if (!(castingType == RE::MagicSystem::CastingType::kFireAndForget ||
			  castingType == RE::MagicSystem::CastingType::kConcentration)) {
			return false;
		}
		const auto delivery = a_spell->GetDelivery();
		if (!(delivery == RE::MagicSystem::Delivery::kAimed ||
			  delivery == RE::MagicSystem::Delivery::kTargetActor ||
			  delivery == RE::MagicSystem::Delivery::kTargetLocation)) {
			return false;
		}
		if (CalculateSpellCost(a_spell) < 1.0f) {
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

	float StaffCraftingMenu::CalculateSpellCost(const RE::SpellItem* a_spell)
	{
		float cost = 0.0f;
		if (!a_spell)
			return cost;

		for (const RE::Effect* const effect : a_spell->effects) {
			const auto baseEffect = effect->baseEffect;
			if (baseEffect->GetArchetype() == RE::EffectSetting::Archetype::kLight &&
				baseEffect->data.flags.none(
					RE::EffectSetting::EffectSettingData::Flag::kNoMagnitude)) {

				RE::Effect tempEffect;
				tempEffect.Copy(effect);
				tempEffect.SetMagnitude(1.0f);
				cost += tempEffect.cost;
			}
			else {
				cost += effect->cost;
			}
		}

		return cost;
	}

	StaffCraftingMenu::SpellLevel StaffCraftingMenu::GetSpellLevel(const RE::SpellItem* a_spell)
	{
		if (!a_spell) {
			return SpellLevel::kNovice;
		}

		auto evilSpell = const_cast<RE::SpellItem*>(a_spell);
		if (!evilSpell) {
			return SpellLevel::kNovice;
		}

		const auto costliest = evilSpell->GetCostliestEffectItem();
		if (!costliest || !costliest->baseEffect) {
			return SpellLevel::kNovice;
		}

		const std::int32_t level = costliest->baseEffect->GetMinimumSkillLevel();
		if (level < 25) {
			return SpellLevel::kNovice;
		}
		else if (level < 50) {
			return SpellLevel::kApprentice;
		}
		else if (level < 75) {
			return SpellLevel::kAdept;
		}
		else if (level < 100) {
			return SpellLevel::kExpert;
		}
		else {
			return SpellLevel::kMaster;
		}
	}

	void StaffCraftingMenu::UpdateTextElements()
	{
		if (craftItemPreview &&
			(currentCategory == Category::Staff || currentCategory == Category::Spell ||
			 selected.Complete())) {

			UpdateItemCard(craftItemPreview.get());
		}
		else if (hasHighlight && highlightIndex < listEntries.size()) {
			listEntries[highlightIndex]->ShowInItemCard(this);
		}
		else if (craftItemPreview) {
			UpdateItemCard(craftItemPreview.get());
		}

		UpdateIngredients();

		if (buttonText.IsArray()) {
			const bool isEnchanting = currentCategory != Category::Recipe;
			const bool canRename = isEnchanting && selected.Complete() &&
				CanSetOverrideName(craftItemPreview.get());

			const auto auxText = canRename ? *"sRenameItem"_gs : "";
			buttonText.SetElement(Button::Aux, auxText);

			const auto craftText = isEnchanting ? *"sCraft"_gs : *"sCreate"_gs;
			buttonText.SetElement(Button::Craft, craftText);

			menu.Invoke("UpdateButtonText");
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
			if (selected.staff && selected.morpholith && selected.spell && craftItemPreview &&
				currentCategory != Category::Recipe) {

				EditItemName();
				return true;
			}
		}

		return false;
	}

	void StaffCraftingMenu::ProcessUpdate([[maybe_unused]] const RE::BSUIMessageData* a_data)
	{
#ifndef SKYRIMVR
		if (!a_data) {
			return;
		}

		const auto& ev = a_data->fixedStr;
		if (ev != "VirtualKeyboardCancelled"sv && ev != "VirtualKeyboardDone"sv) {
			return;
		}

		const auto str = a_data->str ? a_data->str->c_str() : nullptr;
		const bool isValid = a_data->str &&
			RE::BSScaleformManager::GetSingleton()->IsValidName(str);
		const bool cancelled = ev == "VirtualKeyboardCancelled"sv;

		const auto text = (isValid && !cancelled) ? str : nullptr;
		TextEntered(text);

		if (!cancelled && !isValid) {
			RE::ControlMap::GetSingleton()->AllowTextInput(true);

			const bool usingVirtualKeyboard =
				RE::BSWin32SystemUtility::GetSingleton()->isRunningOnSteamDeck;

			if (usingVirtualKeyboard) {
				ShowVirtualKeyboard();
			}
		}
#endif
	}

	void StaffCraftingMenu::TextEntered(const char* a_text)
	{
		if (a_text) {
			customName = a_text;

			if (craftItemPreview) {
				if (const auto extraLists = craftItemPreview->extraLists;
					extraLists && !extraLists->empty()) {
					extraLists->front()->SetOverrideName(a_text);
				}
			}
		}

		RE::ControlMap::GetSingleton()->AllowTextInput(false);
		UpdateTextElements();
	}

	void StaffCraftingMenu::ShowVirtualKeyboard()
	{
		static const auto doneCallback = +[](void*, const char* a_text)
		{
			const auto uiMessageQueue = RE::UIMessageQueue::GetSingleton();
			if (!uiMessageQueue)
				return;

			const auto msgData = RE::UIMessageDataFactory::Create<RE::BSUIMessageData>();
			msgData->fixedStr = "VirtualKeyboardDone"sv;
			msgData->str = new RE::BSString(a_text);

			uiMessageQueue->AddMessage(MENU_NAME, RE::UI_MESSAGE_TYPE::kUpdate, msgData);
		};

		static const auto cancelCallback = +[]()
		{
			const auto uiMessageQueue = RE::UIMessageQueue::GetSingleton();
			if (!uiMessageQueue)
				return;

			const auto msgData = RE::UIMessageDataFactory::Create<RE::BSUIMessageData>();
			msgData->fixedStr = "VirtualKeyboardCancelled"sv;

			uiMessageQueue->AddMessage(MENU_NAME, RE::UI_MESSAGE_TYPE::kUpdate, msgData);
		};

		const auto device = RE::BSInputDeviceManager::GetSingleton()->GetVirtualKeyboard();
		if (device) {
			const RE::BSVirtualKeyboardDevice::kbInfo kbInfo{
				.startingText = suggestedName.c_str(),
				.doneCallback = doneCallback,
				.cancelCallback = cancelCallback,
				.userParam = nullptr,
				.maxChars = "uMaxCustomItemNameLength:Interface"_ini.value_or(20),
			};

			device->Start(&kbInfo);
		}
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
					itemPreview = std::make_unique<RE::InventoryEntryData>();
					itemPreview->DeepCopy(*selected.staff->data);
					itemPreview->countDelta = 1;
					if (itemPreview->IsWorn()) {
						itemPreview->SetWorn(false, true, true);
						itemPreview->SetWorn(false, false, true);
					}
				}
				UpdateItemPreview(std::move(itemPreview));
				UpdateEnchantment();

				UpdateTextElements();
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

			if (CanCraftWithSpell(spellEntry->data)) {
				return true;
			}
			else {
				if (a_showNotification) {
					RE::SendHUDMessage::ShowHUDMessage(*"sEnchantInsufficientCharge"_gs);
				}
				return false;
			}
		}
		else if (a_entry->filterFlag == FilterFlag::Morpholith && selected.spell) {
			const auto itemEntry = static_cast<const ItemEntry*>(a_entry.get());
			const auto& morpholithEntry = itemEntry->data.get();
			if (!morpholithEntry)
				return false;

			const auto morpholithCharge = GetEntryDataSoulCharge(morpholithEntry);
			const auto morpholithCount = morpholithEntry->countDelta;
			const bool canCraft = morpholithCharge > 0.0f
				? morpholithCharge > selected.spell->data->CalculateMagickaCost(nullptr)
				: morpholithCount >= GetSpellHeartstones(selected.spell->data);
			if (canCraft) {
				return true;
			}
			else {
				if (a_showNotification) {
					RE::SendHUDMessage::ShowHUDMessage(*"sEnchantInsufficientCharge"_gs);
				}
				return false;
			}
		}

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
		if (skill >= RE::ActorValue::kOneHanded && skill <= RE::ActorValue::kEnchanting) {
			playerRef->UseSkill(skill.get(), 20.0f);
			UpdateBottomBar(skill.get());
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

		if (const auto enchantableForm = createdItem->As<RE::TESEnchantableForm>();
			enchantableForm && enchantableForm->formEnchanting) {

			RE::PlaySound("UIEnchantingItemCreate");
		}
		else {
			RE::PlaySound("UISmithingCreateGeneric");
		}
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

		const auto invChanges = player->GetInventoryChanges();
		if (!invChanges) {
			return;
		}

		RE::ExtraDataList* staffExtraList = nullptr;
		if (const auto extraLists = selected.staff->data->extraLists;
			extraLists && !extraLists->empty()) {
			staffExtraList = extraLists->front();
		}

		RE::ExtraDataList* const enchantedExtraList = invChanges->EnchantObject(
			staff,
			staffExtraList,
			enchantment,
			static_cast<uint16_t>(chargeAmount));

		if (!enchantedExtraList)
			return;

		const char* newName = "";
		if (customName.empty()) {
			newName = suggestedName.c_str();
		}
		else {
			newName = customName.c_str();
		}
		enchantedExtraList->SetOverrideName(newName);

		if (staffExtraList) {
			const auto wornLeft = staffExtraList->HasType(RE::ExtraDataType::kWornLeft);
			const auto worn = wornLeft || staffExtraList->HasType(RE::ExtraDataType::kWorn);
			if (worn) {
				player->RefreshEquippedActorValueCharge(staff, staffExtraList, wornLeft);
			}
		}

		if (const auto soulGem = selected.morpholith->data->GetObject()->As<RE::TESSoulGem>()) {
			RE::ExtraDataList* morpholithExtraList = nullptr;
			if (const auto extraLists = selected.morpholith->data->extraLists;
				!extraLists->empty()) {
				morpholithExtraList = extraLists->front();
			}

			const auto defaultObjects = RE::BGSDefaultObjectManager::GetSingleton();
			const auto KeywordReusableSoulGem = defaultObjects->GetObject<RE::BGSKeyword>(
				RE::DEFAULT_OBJECT::kKeywordReusableSoulGem);
			const bool usingReusableSoulGem = soulGem->HasKeyword(KeywordReusableSoulGem);

			if (usingReusableSoulGem) {
				if (morpholithExtraList) {
					if (const auto extraSoul = morpholithExtraList->GetByType<RE::ExtraSoul>()) {
						extraSoul->soul = RE::SOUL_LEVEL::kNone;
					}
				}
			}
			else {
				player->RemoveItem(
					selected.morpholith->data->GetObject(),
					1,
					RE::ITEM_REMOVE_REASON::kRemove,
					morpholithExtraList,
					nullptr);
			}
		}
		else {
			player->RemoveItem(
				selected.morpholith->data->GetObject(),
				GetSpellHeartstones(selected.spell->data),
				RE::ITEM_REMOVE_REASON::kRemove,
				nullptr,
				nullptr);
		}

		player->AddChange(
			RE::TESObjectREFR::ChangeFlags::kInventory |
			RE::TESObjectREFR::ChangeFlags::kItemExtraData |
			RE::TESObjectREFR::ChangeFlags::kLeveledInventory);

		RE::SendHUDMessage::ShowInventoryChangeMessage(staff, 1, true, true, newName);

		const auto skill = workbench->workBenchData.usesSkill;
		if (skill >= RE::ActorValue::kOneHanded && skill <= RE::ActorValue::kEnchanting) {
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

	void StaffCraftingMenu::EditItemName()
	{
		if (!CanSetOverrideName(craftItemPreview.get())) {
			return;
		}

		RE::ControlMap::GetSingleton()->AllowTextInput(true);

		const bool usingVirtualKeyboard = IF_SKYRIMSE(
			RE::BSWin32SystemUtility::GetSingleton()->isRunningOnSteamDeck,
			false);

		if (usingVirtualKeyboard) {
			ShowVirtualKeyboard();
		}
		else {
			const auto maxString = "uMaxCustomItemNameLength:Interface"_ini.value_or(32);
			menu.Invoke(
				"EditItemName",
				std::to_array<RE::GFxValue>({ suggestedName.c_str(), maxString }));
		}
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

		if (morpholith) {
			morpholith->selected = false;
		}

		morpholith.reset();
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
