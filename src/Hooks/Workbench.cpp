#include "Workbench.h"

#include "RE/Offset.h"
#include "UI/StaffCraftingMenu.h"

namespace Hooks
{
	void Workbench::Install()
	{
		auto hook = REL::Relocation<std::uintptr_t>(
			RE::Offset::TESObjectREFR::ActivateCraftingWorkbench,
			0xE);
		REL::make_pattern<"E8">().match_or_fail(hook.address());
		auto& trampoline = SKSE::GetTrampoline();
		_IsFurniture = trampoline.write_call<5>(hook.address(), &Workbench::CheckFurniture);
	}

	[[nodiscard]] static bool IsStaffCraftingWorkbench(const RE::TESObjectREFR* a_refr)
	{
		if (!a_refr) {
			return false;
		}

		const auto baseForm = a_refr->GetBaseObject();
		const auto keywordForm = skyrim_cast<const RE::BGSKeywordForm*>(baseForm);
		if (!keywordForm) {
			return false;
		}

		const auto dataHandler = RE::TESDataHandler::GetSingleton();
		const auto idx_Dragonborn = dataHandler
			? dataHandler->GetModIndex("Dragonborn.esm"sv)
			: std::nullopt;

		if (!idx_Dragonborn) {
			return false;
		}

		const RE::FormID DLC2StaffEnchanter = (*idx_Dragonborn << 24) | 0x17738;
		return keywordForm->HasKeyword(DLC2StaffEnchanter);
	}

	static void ShowStaffCraftingMenu()
	{
		const auto uiMessageQueue = RE::UIMessageQueue::GetSingleton();
		assert(uiMessageQueue);
		uiMessageQueue
			->AddMessage(UI::StaffCraftingMenu::MENU_NAME, RE::UI_MESSAGE_TYPE::kShow, nullptr);
	}

	bool Workbench::CheckFurniture(RE::TESObjectREFR* a_refr)
	{
		if (IsStaffCraftingWorkbench(a_refr)) {
			ShowStaffCraftingMenu();
			return false;
		}
		else {
			return _IsFurniture(a_refr);
		}
	}
}
