#include "StaffCraftingMenu_Callbacks.h"
#include "StaffCraftingMenu.h"

namespace UI
{
	void StaffCraftingMenu_Callbacks::Register(CallbackProcessor* a_processor)
	{
		a_processor->Process("SetSelectedItem", &SetSelectedItem);
		a_processor->Process("SetSelectedCategory", &SetSelectedCategory);
		a_processor->Process("ChooseItem", &ChooseItem);
		a_processor->Process("ShowItem3D", &ShowItem3D);
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

	void StaffCraftingMenu_Callbacks::SetSelectedItem(const RE::FxDelegateArgs& a_params)
	{
		if (a_params.GetArgCount() < 1) {
			return;
		}

		const auto index = a_params[0].GetNumber();
		const auto menu = static_cast<StaffCraftingMenu*>(a_params.GetHandler());

		if (menu->hasHighlight = index >= 0) {
			auto pos = static_cast<std::uint32_t>(index);
			pos = std::min(pos, menu->listEntries.size() - 1);
			if (pos != menu->highlightIndex) {
				RE::PlaySound("UIMenuFocus");
				menu->highlightIndex = pos;
			}
		}

		menu->UpdateInterface();
	}

	void StaffCraftingMenu_Callbacks::SetSelectedCategory(const RE::FxDelegateArgs& a_params)
	{
		if (a_params.GetArgCount() < 1) {
			return;
		}

		// TODO: see 51419
	}

	void StaffCraftingMenu_Callbacks::ChooseItem(const RE::FxDelegateArgs& a_params)
	{
		if (a_params.GetArgCount() < 1) {
			return;
		}

		const auto index = a_params[0].GetNumber();
		const auto menu = static_cast<StaffCraftingMenu*>(a_params.GetHandler());

		menu->ChooseItem(static_cast<std::uint32_t>(index));
	}

	void StaffCraftingMenu_Callbacks::ShowItem3D(const RE::FxDelegateArgs& a_params)
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
