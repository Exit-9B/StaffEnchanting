#include "ScaleformExtendedData.h"

namespace SKSE
{
	void scaleformExtend::CommonItemData(RE::GFxValue& a_fxVal, const RE::TESForm* a_form)
	{
		if (!a_form || !a_fxVal.IsObject())
			return;

		a_fxVal.SetMember("formType", a_form->GetFormType());
		a_fxVal.SetMember("formId", a_form->GetFormID());
	}

	void scaleformExtend::StandardItemData(
		RE::GFxValue& a_fxVal,
		const RE::TESForm* a_form,
		const RE::InventoryEntryData* a_entry)
	{
		if (!a_form || !a_fxVal.IsObject())
			return;

		switch (a_form->GetFormType()) {
		case RE::FormType::Armor:
		{
			const auto armor = static_cast<const RE::TESObjectARMO*>(a_form);
			a_fxVal.SetMember("partMask", armor->GetSlotMask());
			a_fxVal.SetMember("weightClass", armor->GetArmorType());
		} break;

		case RE::FormType::Ammo:
		{
			const auto ammo = static_cast<const RE::TESAmmo*>(a_form);
			a_fxVal.SetMember("flags", ammo->data.flags.underlying());
		} break;

		case RE::FormType::Weapon:
		{
			const auto weapon = static_cast<const RE::TESObjectWEAP*>(a_form);
			a_fxVal.SetMember("subType", weapon->GetWeaponType());
			a_fxVal.SetMember("weaponType", weapon->GetWeaponType());
			a_fxVal.SetMember("speed", weapon->GetSpeed());
			a_fxVal.SetMember("reach", weapon->GetReach());
			a_fxVal.SetMember("stagger", weapon->GetStagger());
			a_fxVal.SetMember("critDamage", weapon->GetCritDamage());
			a_fxVal.SetMember("minRange", weapon->GetMinRange());
			a_fxVal.SetMember("maxRange", weapon->GetMaxRange());
			a_fxVal.SetMember("baseDamage", weapon->GetAttackDamage());

			if (const auto equipSlot = weapon->GetEquipSlot()) {
				a_fxVal.SetMember("equipSlot", equipSlot->GetFormID());
			}
		} break;

		case RE::FormType::SoulGem:
		{
			const auto soulGem = static_cast<const RE::TESSoulGem*>(a_form);
			a_fxVal.SetMember("gemSize", soulGem->GetMaximumCapacity());
			a_fxVal.SetMember(
				"soulSize",
				a_entry ? a_entry->GetSoulLevel() : soulGem->GetContainedSoul());
		} break;

		case RE::FormType::AlchemyItem:
		{
			const auto potion = static_cast<const RE::AlchemyItem*>(a_form);
			a_fxVal.SetMember("flags", potion->data.flags.underlying());
		} break;

		case RE::FormType::Book:
		{
			const auto book = static_cast<const RE::TESObjectBOOK*>(a_form);
			a_fxVal.SetMember("flags", book->data.flags.underlying());
			a_fxVal.SetMember("bookType", book->data.type.underlying());

			switch (book->data.GetSanitizedType()) {
			case RE::OBJ_BOOK::Flag::kAdvancesActorValue:
			{
				a_fxVal.SetMember("teachesSkill", book->data.teaches.actorValueToAdvance);
			} break;

			case RE::OBJ_BOOK::Flag::kTeachesSpell:
			{
				const auto& spell = book->data.teaches.spell;
				a_fxVal.SetMember("teachesSpell", spell ? spell->GetFormID() : -1);
			} break;
			}
		} break;
		}
	}

	void scaleformExtend::MagicItemData(
		RE::GFxValue& a_fxVal,
		const RE::TESForm* a_form,
		bool a_extra,
		bool a_recursive)
	{
		if (!a_form || !a_fxVal.IsObject())
			return;

		switch (a_form->GetFormType()) {
		case RE::FormType::Spell:
		case RE::FormType::Scroll:
		case RE::FormType::Ingredient:
		case RE::FormType::AlchemyItem:
		case RE::FormType::Enchantment:
		{
			const auto magicItem = static_cast<const RE::MagicItem*>(a_form);
			if (!magicItem->fullName.empty())
				a_fxVal.SetMember("spellName", magicItem->fullName.data());

			const auto effect = const_cast<RE::MagicItem*>(magicItem)->GetCostliestEffectItem();
			if (effect && effect->baseEffect) {
				a_fxVal.SetMember("magnitude", effect->effectItem.magnitude);
				a_fxVal.SetMember("duration", effect->effectItem.duration);
				a_fxVal.SetMember("area", effect->effectItem.area);

				MagicItemData(
					a_fxVal,
					effect->baseEffect,
					a_recursive && a_extra,
					a_recursive);
			}

			if (const auto spell = a_form->As<RE::SpellItem>()) {
				a_fxVal.SetMember("spellType", spell->GetSpellType());
				a_fxVal.SetMember("trueCost", spell->data.costOverride);

				if (const auto equipSlot = spell->GetEquipSlot()) {
					a_fxVal.SetMember("equipSlot", equipSlot->GetFormID());
				}
			}
		} break;

		case RE::FormType::MagicEffect:
		{
			const auto magicEffect = static_cast<const RE::EffectSetting*>(a_form);
			if (!magicEffect->fullName.empty())
				a_fxVal.SetMember("effectName", magicEffect->fullName.data());

			a_fxVal.SetMember("subType", magicEffect->GetMagickSkill());
			a_fxVal.SetMember("effectFlags", magicEffect->data.flags.underlying());
			a_fxVal.SetMember("school", magicEffect->GetMagickSkill());
			a_fxVal.SetMember("skillLevel", magicEffect->GetMinimumSkillLevel());
			a_fxVal.SetMember("archetype", magicEffect->GetArchetype());
			a_fxVal.SetMember("deliveryType", magicEffect->data.delivery);
			a_fxVal.SetMember("castTime", magicEffect->data.spellmakingChargeTime);
			a_fxVal.SetMember("delayTime", magicEffect->data.aiDelayTimer);
			a_fxVal.SetMember("actorValue", magicEffect->data.primaryAV);
			a_fxVal.SetMember("castType", magicEffect->data.castingType);
			a_fxVal.SetMember("magicType", magicEffect->data.resistVariable);
		} break;

		case RE::FormType::Shout:
		{
			const auto shout = static_cast<const RE::TESShout*>(a_form);
			if (!shout->fullName.empty())
				a_fxVal.SetMember("fullName", shout->fullName.data());
		} break;
		}
	}

	void scaleformExtend::ItemInfoData(
		RE::GFxValue& a_fxVal,
		const RE::InventoryEntryData* a_entry)
	{
		if (!a_entry || !a_fxVal.IsObject())
			return;

		const auto player = RE::PlayerCharacter::GetSingleton();

		a_fxVal.SetMember("value", a_entry->GetValue());
		a_fxVal.SetMember("weight", a_entry->GetWeight());
		a_fxVal.SetMember(
			"isStolen",
			!const_cast<RE::InventoryEntryData*>(a_entry)->IsOwnedBy(player, true));
	}
}
