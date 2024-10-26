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

	[[nodiscard]] inline std::int32_t GetInventoryItemCount(
		const RE::TESObjectREFR* a_refr,
		bool a_isViewingContainer = false,
		bool a_playable = true)
	{
		using func_t = decltype(&GetInventoryItemCount);
		REL::Relocation<func_t> func{ RE::Offset::TESObjectREFR::GetInventoryItemCount };
		return func(a_refr, a_isViewingContainer, a_playable);
	}

	[[nodiscard]] inline RE::InventoryEntryData* GetInventoryItemAt(
		const RE::TESObjectREFR* a_refr,
		std::int32_t a_index,
		bool a_isViewingContainer = false)
	{
		using func_t = decltype(&GetInventoryItemAt);
		REL::Relocation<func_t> func{ RE::Offset::TESObjectREFR::GetInventoryItemAt };
		return func(a_refr, a_index, a_isViewingContainer);
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

	inline RE::ExtraDataList* EnchantObject(
		RE::InventoryChanges* a_inventoryChanges,
		RE::TESBoundObject* a_obj,
		RE::ExtraDataList* a_extraList,
		RE::EnchantmentItem* a_enchantment,
		uint16_t a_charge)
	{
		using func_t = decltype(&EnchantObject);
		static REL::Relocation<func_t> func{ RE::Offset::InventoryChanges::EnchantObject };
		return func(a_inventoryChanges, a_obj, a_extraList, a_enchantment, a_charge);
	}

	inline void RefreshEquippedActorValueCharge(
		RE::Actor* a_actor,
		const RE::TESForm* a_object,
		const RE::ExtraDataList* a_extraList,
		bool a_isLeft)
	{
		using func_t = decltype(&RefreshEquippedActorValueCharge);
		static REL::Relocation<func_t> func{ RE::Offset::Actor::RefreshEquippedActorValueCharge };
		return func(a_actor, a_object, a_extraList, a_isLeft);
	}
}
