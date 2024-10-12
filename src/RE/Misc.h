#pragma once

#include "Offset.h"

namespace RE
{
	[[nodiscard]] inline const char* GetActorValueName(RE::ActorValue a_actorValue)
	{
		using func_t = decltype(&GetActorValueName);
		static REL::Relocation<func_t> func{ RE::Offset::GetActorValueName };
		return func(a_actorValue);
	}

	[[nodiscard]] inline RE::IUIMessageData* CreateUIMessageData(const RE::BSFixedString& a_name)
	{
		using func_t = decltype(&CreateUIMessageData);
		static REL::Relocation<func_t> func{ RE::Offset::UIMessageDataFactory::Create };
		return func(a_name);
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
