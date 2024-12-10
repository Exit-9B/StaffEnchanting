#include "Workbench.h"

#include "RE/Offset.h"
#include "common/InterfaceStrings.h"

namespace Hooks
{
	void Workbench::Install()
	{
		auto hook = util::GameAddress(RE::Offset::TESObjectREFR::ActivateCraftingWorkbench, 0xE);
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
		const auto furniture = baseForm->As<RE::TESFurniture>();
		if (!furniture) {
			return false;
		}

		const auto dataHandler = RE::TESDataHandler::GetSingleton();
		const auto DLC2StaffEnchanter = dataHandler->LookupForm<RE::BGSKeyword>(
			0x17738,
			"Dragonborn.esm"sv);

		return furniture->HasKeyword(DLC2StaffEnchanter);
	}

	static void ShowStaffCraftingMenu()
	{
		const auto uiMessageQueue = RE::UIMessageQueue::GetSingleton();
		assert(uiMessageQueue);
		uiMessageQueue
			->AddMessage(InterfaceStrings::StaffCraftingMenu, RE::UI_MESSAGE_TYPE::kShow, nullptr);
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
