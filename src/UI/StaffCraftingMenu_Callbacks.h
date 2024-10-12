#pragma once

namespace UI
{
	struct StaffCraftingMenu_Callbacks
	{
		using CallbackProcessor = RE::FxDelegateHandler::CallbackProcessor;

		static void Register(CallbackProcessor* a_processor);

		static void SetSelectedItem(const RE::FxDelegateArgs& a_params);
		static void SetSelectedCategory(const RE::FxDelegateArgs& a_params);
		static void ChooseItem(const RE::FxDelegateArgs& a_params);
		static void ShowItem3D(const RE::FxDelegateArgs& a_params);
	};
}
