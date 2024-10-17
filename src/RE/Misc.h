#pragma once

#include "Offset.h"

namespace RE
{
	namespace SendHUDMessage
	{
		inline void SetHUDMode(const char* a_mode, bool a_push)
		{
			const auto uiMessageQueue = RE::UIMessageQueue::GetSingleton();
			const auto interfaceStrings = RE::InterfaceStrings::GetSingleton();
			if (const auto data = static_cast<RE::HUDData*>(
					uiMessageQueue->CreateUIMessageData(interfaceStrings->hudData))) {
				data->unk40 = a_push;
				data->text = a_mode;
				data->type = static_cast<RE::HUDData::Type>(23);
				uiMessageQueue
					->AddMessage(RE::HUDMenu::MENU_NAME, RE::UI_MESSAGE_TYPE::kUpdate, data);
			}
		}

		inline void PushHudMode(const char* a_mode)
		{
			SetHUDMode(a_mode, true);
		}

		inline void PopHudMode(const char* a_mode)
		{
			SetHUDMode(a_mode, false);
		}
	}

	namespace UIMessageDataFactory
	{
		[[nodiscard]] inline RE::IUIMessageData* Create(const RE::BSFixedString& a_name)
		{
			using func_t = decltype(&Create);
			static REL::Relocation<func_t> func{ RE::Offset::UIMessageDataFactory::Create };
			return func(a_name);
		}
	}

	[[nodiscard]] inline const char* GetActorValueName(RE::ActorValue a_actorValue)
	{
		using func_t = decltype(&GetActorValueName);
		static REL::Relocation<func_t> func{ RE::Offset::GetActorValueName };
		return func(a_actorValue);
	}

	[[nodiscard]] inline bool CanBeCreatedOnWorkbench(
		const RE::BGSConstructibleObject* a_obj,
		const RE::TESFurniture* a_workbench,
		bool a_checkConditions)
	{
		if (!a_workbench || !a_workbench->HasKeyword(a_obj->benchKeyword)) {
			return false;
		}

		if (a_obj->conditions.head && a_checkConditions) {
			const auto playerRef = RE::PlayerCharacter::GetSingleton();
			return a_obj->conditions.IsTrue(playerRef, playerRef);
		}
		else {
			return true;
		}
	}

	[[nodiscard]] inline RE::BSFurnitureMarkerNode* GetFurnitureMarkerNode(RE::NiAVObject* a_root)
	{
		using func_t = decltype(&GetFurnitureMarkerNode);
		static REL::Relocation<func_t> func{ RE::Offset::GetFurnitureMarkerNode };
		return func(a_root);
	}

	inline void ClearFurniture(RE::AIProcess* a_process)
	{
		using func_t = decltype(&ClearFurniture);
		static REL::Relocation<func_t> func{ RE::Offset::AIProcess::ClearFurniture };
		return func(a_process);
	}

	[[nodiscard]] inline std::int32_t GetItemCount(
		const RE::TESContainer* a_container,
		const RE::TESBoundObject* a_object)
	{
		std::int32_t count = 0;
		for (const RE::ContainerObject* const entry :
			 std::span(a_container->containerObjects, a_container->numContainerObjects)) {
			if (entry->obj == a_object) {
				count += entry->count;
			}
		}
		return count;
	}

	[[nodiscard]] inline std::int32_t GetCountDelta(
		const RE::TESBoundObject* a_object,
		const RE::InventoryChanges* a_invChanges,
		std::predicate<const RE::InventoryEntryData*> auto a_itemFilter)
	{
		const auto container = a_invChanges->owner ? a_invChanges->owner->GetContainer() : nullptr;
		std::int32_t count = container ? std::max(0, GetItemCount(container, a_object)) : 0;
		if (a_invChanges->entryList) {
			auto& entryList = *a_invChanges->entryList;
			const auto it = std::ranges::find_if(
				entryList,
				[a_object](const RE::InventoryEntryData* a_entry)
				{
					return a_entry && a_entry->object == a_object;
				});

			if (it != std::ranges::end(entryList)) {
				if (a_itemFilter(*it)) {
					count += (*it)->countDelta;
				}
			}
		}
		return count;
	}
}
