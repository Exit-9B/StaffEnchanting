#include "StaffCraftingMenu.h"

namespace UI
{
	void StaffCraftingMenu::RegisterFuncs(CallbackProcessor* a_processor)
	{
		a_processor->Process("GetScreenDimensions", &StaffCraftingMenu::GetScreenDimensions);
		a_processor->Process("SetSelectedItem", &StaffCraftingMenu::SetSelectedItem);
		a_processor->Process("SetSelectedCategory", &StaffCraftingMenu::SetSelectedCategory);
		a_processor->Process("ChooseItem", &StaffCraftingMenu::ChooseItem);
		a_processor->Process("ShowItem3D", &StaffCraftingMenu::ShowItem3D);
		a_processor->Process("CanFadeItemInfo", &StaffCraftingMenu::CanFadeItemInfo);
		a_processor->Process("EndItemRename", &StaffCraftingMenu::EndItemRename);
		a_processor->Process("CloseMenu", &StaffCraftingMenu::CloseMenu);
		a_processor->Process("CraftButtonPress", &StaffCraftingMenu::CraftButtonPress);
		a_processor->Process("StartMouseRotation", &StaffCraftingMenu::StartMouseRotation);
		a_processor->Process("StopMouseRotation", &StaffCraftingMenu::StopMouseRotation);
		a_processor->Process("AuxButtonPress", &StaffCraftingMenu::AuxButtonPress);
	}

	void StaffCraftingMenu::GetScreenDimensions(const RE::FxDelegateArgs& a_params)
	{
		const auto graphicsState = RE::BSGraphics::State::GetSingleton();
		a_params.Respond(graphicsState->screenWidth, graphicsState->screenHeight);
	}

	void StaffCraftingMenu::SetSelectedItem(const RE::FxDelegateArgs& a_params)
	{
		if (a_params.GetArgCount() < 1) {
			return;
		}

		const auto index = a_params[0].GetNumber();
		const auto menu = static_cast<StaffCraftingMenu*>(a_params.GetHandler());

		menu->hasHighlight = index >= 0;
		if (index >= 0) {
			auto pos = static_cast<std::uint32_t>(index);
			pos = std::min(pos, menu->listEntries.size() - 1);
			if (pos != menu->highlightIndex) {
				RE::PlaySound("UIMenuFocus");
				menu->highlightIndex = pos;
			}
		}

		menu->UpdateTextElements();
	}

	bool StaffCraftingMenu::IsCategoryEmpty(Category a_category) const
	{
		RE::GFxValue entryList;
		if (!itemList.IsObject() || !itemList.GetMember("entryList", &entryList) ||
			!entryList.IsArray()) {
			return false;
		}

		for (const auto i : std::views::iota(0u, entryList.GetArraySize())) {
			RE::GFxValue entry;
			if (!entryList.GetElement(i, &entry)) {
				continue;
			}

			RE::GFxValue filterFlag;
			if (!entry.IsObject() || !entry.GetMember("filterFlag", &filterFlag)) {
				continue;
			}

			const auto filter = util::to_underlying(filters[a_category]);
			if (filterFlag.IsNumber() && (filterFlag.GetUInt() & filter) == filter) {
				return false;
			}
		}

		return true;
	}

	void StaffCraftingMenu::SetSelectedCategory(const RE::FxDelegateArgs& a_params)
	{
		if (a_params.GetArgCount() < 1) {
			return;
		}

		const auto category = static_cast<Category>(a_params[0].GetNumber());
		const auto menu = static_cast<StaffCraftingMenu*>(a_params.GetHandler());

		if (category >= Category::TOTAL) {
			return;
		}

		if (category == Category::Recipe && menu->IsCategoryEmpty(Category::Recipe)) {
			RE::GFxValue categoriesList;
			menu->inventoryLists.GetMember("CategoriesList", &categoriesList);
			if (categoriesList.IsObject()) {
				categoriesList
					.Invoke("onItemPress", std::to_array<RE::GFxValue>({ Category::Staff, 0 }));
				return;
			}
		}

		const auto previousCategory = menu->currentCategory;
		menu->currentCategory = category;
		menu->UpdateTextElements();

		const auto inventory3D = RE::Inventory3DManager::GetSingleton();
		assert(inventory3D);
		inventory3D->unk159 = menu->currentCategory != Category::Spell;
		inventory3D->unk158 = menu->currentCategory != Category::Spell;
		if (menu->currentCategory != Category::Recipe) {
			if (menu->craftItemPreview && previousCategory == Category::Recipe) {
				menu->menu.Invoke("FadeInfoCard", std::to_array<RE::GFxValue>({ false }));
				menu->UpdateItemCard(menu->craftItemPreview.get());
				inventory3D->UpdateItem3D(menu->craftItemPreview.get());
			}
		}
		else {
			menu->menu.Invoke("FadeInfoCard", std::to_array<RE::GFxValue>({ true }));
			inventory3D->Clear3D();
		}

		if (category == Category::Recipe || previousCategory == Category::Recipe) {
			menu->UpdateIngredients();
		}
	}

	void StaffCraftingMenu::ChooseItem(const RE::FxDelegateArgs& a_params)
	{
		if (a_params.GetArgCount() < 1) {
			return;
		}

		const auto index = a_params[0].GetNumber();
		const auto menu = static_cast<StaffCraftingMenu*>(a_params.GetHandler());

		menu->ChooseItem(static_cast<std::uint32_t>(index));
	}

	void StaffCraftingMenu::ShowItem3D(const RE::FxDelegateArgs& a_params)
	{
		if (a_params.GetArgCount() < 1) {
			return;
		}

		const auto show = a_params[0].GetBool();
		const auto menu = static_cast<StaffCraftingMenu*>(a_params.GetHandler());

		if (!menu->craftItemPreview || menu->currentCategory == Category::Recipe) {
			if (menu->highlightIndex < menu->listEntries.size()) {
				menu->listEntries[menu->highlightIndex]->ShowItem3D(show);
			}
			else {
				const auto inventory3D = RE::Inventory3DManager::GetSingleton();
				assert(inventory3D);
				inventory3D->Clear3D();
			}
		}
	}

	void StaffCraftingMenu::CanFadeItemInfo(const RE::FxDelegateArgs& a_params)
	{
		const auto menu = static_cast<StaffCraftingMenu*>(a_params.GetHandler());
		a_params.Respond(!menu->craftItemPreview || menu->currentCategory == Category::Recipe);
	}

	void StaffCraftingMenu::EndItemRename(const RE::FxDelegateArgs& a_params)
	{
		if (a_params.GetArgCount() < 1) {
			return;
		}

		const auto menu = static_cast<StaffCraftingMenu*>(a_params.GetHandler());
		const auto useNewName = a_params[0].GetBool();

		if (useNewName && a_params.GetArgCount() >= 2) {
			const auto newName = a_params[1].GetString();
			const auto scaleformManager = RE::BSScaleformManager::GetSingleton();
			if (newName && scaleformManager->IsValidName(newName)) {
				menu->customName = newName;

				if (menu->craftItemPreview) {
					if (const auto extraLists = menu->craftItemPreview->extraLists;
						extraLists && !extraLists->empty()) {
						extraLists->front()->SetOverrideName(newName);
					}
				}
			}
		}

		const auto controlMap = RE::ControlMap::GetSingleton();
		controlMap->AllowTextInput(false);

		menu->UpdateTextElements();
	}

	void StaffCraftingMenu::CloseMenu(const RE::FxDelegateArgs&)
	{
		const auto userEvents = RE::UserEvents::GetSingleton();
		RE::BSUIMessageData::SendUIStringMessage(
			MENU_NAME,
			RE::UI_MESSAGE_TYPE::kUserEvent,
			userEvents->cancel);
	}

	void StaffCraftingMenu::CraftButtonPress(const RE::FxDelegateArgs&)
	{
		const auto userEvents = RE::UserEvents::GetSingleton();
		RE::BSUIMessageData::SendUIStringMessage(
			MENU_NAME,
			RE::UI_MESSAGE_TYPE::kUserEvent,
			userEvents->xButton);
	}

	void StaffCraftingMenu::StartMouseRotation(const RE::FxDelegateArgs&)
	{
		RE::Inventory3DManager::SetMouseRotation(true);
	}

	void StaffCraftingMenu::StopMouseRotation(const RE::FxDelegateArgs&)
	{
		RE::Inventory3DManager::SetMouseRotation(false);
	}

	void StaffCraftingMenu::AuxButtonPress(const RE::FxDelegateArgs& a_params)
	{
		const auto menu = static_cast<StaffCraftingMenu*>(a_params.GetHandler());
		menu->EditItemName();
	}
}
