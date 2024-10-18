#pragma once

#include "Offset.h"

namespace RE
{
	namespace InventoryUtils
	{
		inline bool QuestItemFilter(const RE::InventoryEntryData* a_entry)
		{
			return !a_entry->IsQuestObject();
		}
	}

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

	template <std::invocable<RE::IMessageBoxCallback::Message> F>
	[[nodiscard]] RE::BSTSmartPointer<RE::IMessageBoxCallback> MakeMessageBoxCallback(
		F&& a_callback)
	{
		class Callback : public RE::IMessageBoxCallback
		{
		public:
			Callback(F&& a_fn) : fn{ std::forward<F>(a_fn) } {}

			void Run(Message a_msg) override { fn(a_msg); }

		private:
			std::decay_t<F> fn;
		};

		return RE::make_smart<Callback>(std::forward<F>(a_callback));
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

	[[nodiscard]] inline std::int32_t GetObjectCount(
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

	[[nodiscard]] inline std::int32_t GetCount(
		const RE::InventoryChanges* a_invChanges,
		const RE::TESBoundObject* a_object,
		std::predicate<const RE::InventoryEntryData*> auto a_itemFilter)
	{
		const auto container = a_invChanges->owner ? a_invChanges->owner->GetContainer() : nullptr;
		std::int32_t count = container ? std::max(0, GetObjectCount(container, a_object)) : 0;
		if (a_invChanges->entryList) {
			const RE::InventoryEntryData* objEntry = nullptr;
			for (const auto* const entry : *a_invChanges->entryList) {
				if (entry && entry->object == a_object) {
					objEntry = entry;
					break;
				}
			}

			if (objEntry) {
				if (a_itemFilter(objEntry)) {
					count += objEntry->countDelta;
				}
			}
		}
		return count;
	}
}
