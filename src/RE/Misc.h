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

	namespace UIMessageDataFactory
	{
		[[nodiscard]] inline RE::IUIMessageData* Create(const RE::BSFixedString& a_name)
		{
			using func_t = decltype(&Create);
			static REL::Relocation<func_t> func{ RE::Offset::UIMessageDataFactory::Create };
			return func(a_name);
		}
	}

	namespace UIUtils
	{
		inline void PlayMenuSound(const RE::BGSSoundDescriptorForm* a_descriptor)
		{
			using func_t = decltype(&PlayMenuSound);
			static REL::Relocation<func_t> func{ RE::Offset::UIUtils::PlayMenuSound };
			return func(a_descriptor);
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

	[[nodiscard]] inline RE::BGSSoundDescriptorForm* GetNoActivationSound()
	{
		const auto defaultObjects = RE::BGSDefaultObjectManager::GetSingleton();
		const auto sound = defaultObjects->GetObject<RE::BGSSoundDescriptorForm>(
			RE::DEFAULT_OBJECT::kNoActivationSound);

		if (sound) {
			const auto audioManager = RE::BSAudioManager::GetSingleton();
			audioManager->PrecacheDescriptor(sound, 16);
		}

		return sound;
	}

	inline void SetOverrideName(RE::ExtraDataList* a_extraList, const char* a_name)
	{
		auto textData = a_extraList->GetByType<RE::ExtraTextDisplayData>();
		if (!textData) {
			textData = new RE::ExtraTextDisplayData();
			a_extraList->Add(textData);
		}

		if (!textData->displayNameText && !textData->ownerQuest) {
			textData->SetName(a_name);
		}
	}
}
