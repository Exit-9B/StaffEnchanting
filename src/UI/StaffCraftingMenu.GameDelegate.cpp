#include "StaffCraftingMenu.h"

namespace UI
{
	void StaffCraftingMenu::RegisterFuncs(CallbackProcessor* a_processor)
	{
		a_processor->Process("SetSelectedItem", &StaffCraftingMenu::SetSelectedItem);
		a_processor->Process("SetSelectedCategory", &StaffCraftingMenu::SetSelectedCategory);
		a_processor->Process("ChooseItem", &StaffCraftingMenu::ChooseItem);
		a_processor->Process("ShowItem3D", &StaffCraftingMenu::ShowItem3D);
		// TODO: figure out which callback functions need to be here
		// CanFadeItemInfo
		// EndItemRename
		// SliderClose
		// CalculateCharge
		// CloseMenu
		// CraftButtonPress
		// StartMouseRotation
		// StopMouseRotation
		// AuxButtonPress
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

		menu->UpdateInterface();
	}

	void StaffCraftingMenu::SetSelectedCategory(const RE::FxDelegateArgs& a_params)
	{
		if (a_params.GetArgCount() < 1) {
			return;
		}

		const auto category = static_cast<Category::INDEX>(a_params[0].GetNumber());
		const auto menu = static_cast<StaffCraftingMenu*>(a_params.GetHandler());

		if (category >= Category::TOTAL) {
			return;
		}

		const auto previousCategory = menu->currentCategory;
		menu->currentCategory = category;
		menu->UpdateInterface();

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

		if (!menu->craftItemPreview) {
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
}
