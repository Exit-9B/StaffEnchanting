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
}
