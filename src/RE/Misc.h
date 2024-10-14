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
}
