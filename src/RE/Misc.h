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

	[[nodiscard]] inline bool IsQuestItem(const RE::ExtraAliasInstanceArray* a_extraAliases)
	{
		for (const auto alias : a_extraAliases->aliases) {
			if (!alias || !alias->alias) continue;

			if (alias->alias->flags.all(RE::BGSBaseAlias::FLAGS::kQuestObject)) {
				return true;
			}
		}

		return false;
	}

	[[nodiscard]] inline bool IsQuestItem(const RE::ExtraDataList* a_extraList)
	{
		const auto extraRef = a_extraList->GetByType<RE::ExtraReferenceHandle>();
		const auto refHandle = extraRef ? extraRef->containerRef : ObjectRefHandle();
		const auto ref = refHandle.get();

		const auto extraList = ref ? std::addressof(ref->extraList) : a_extraList;
		const auto extraAliases = extraList->GetByType<RE::ExtraAliasInstanceArray>();
		return extraAliases && RE::IsQuestItem(extraAliases);
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
}
