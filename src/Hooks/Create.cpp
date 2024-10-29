#include "Create.h"
#include "RE/Offset.h"

#include <xbyak/xbyak.h>

#undef GetObject

namespace Hooks
{
	void Create::Install()
	{
#ifndef SKYRIMVR
		auto hook = util::GameAddress(
			RE::Offset::MagicItemCreationHelpers::CreateNewEnchantment,
			0x6B);

		static constexpr std::size_t size = 0x44;

		REL::safe_fill(hook.address(), REL::NOP, size);

		struct Patch : Xbyak::CodeGenerator
		{
			Patch() : Xbyak::CodeGenerator(size)
			{
				Xbyak::Label funcLbl;
				Xbyak::Label retnLbl;

				movzx(r8d, bl);
				mov(rdx, rsi);
				mov(rcx, rdi);
				call(ptr[rip + funcLbl]);
				jmp(retnLbl, T_SHORT);
				nop(3);

				L(funcLbl);
				dq(std::bit_cast<std::uintptr_t>(&Create::InitEnchantment));
				nop(size - 0x1D);
				L(retnLbl);
			}
		} patch{};
		assert(patch.getSize() == size);

		REL::safe_write(hook.address(), patch.getCode(), patch.getSize());
#else
		auto hook = util::GameAddress(RE::Offset::BGSCreatedObjectManager::InitEnchantment, 0x26);

		static constexpr std::size_t size = 0x5E;

		REL::safe_fill(hook.address(), REL::NOP, size);

		struct Patch : Xbyak::CodeGenerator
		{
			Patch() : Xbyak::CodeGenerator(size)
			{
				Xbyak::Label funcLbl;
				Xbyak::Label retnLbl;

				call(ptr[rip + funcLbl]);
				jmp(retnLbl, T_SHORT);
				nop(2);

				L(funcLbl);
				dq(std::bit_cast<std::uintptr_t>(&Create::InitEnchantment));
				nop(size - 0x12);
				L(retnLbl);
			}
		} patch{};
		assert(patch.getSize() == size);

		REL::safe_write(hook.address(), patch.getCode(), patch.getSize());
#endif
	}

	void Create::InitEnchantment(
		RE::EnchantmentItem& a_enchantment,
		const RE::BSTArray<RE::Effect>& a_effects,
		bool a_isWeapon)
	{
		const auto defaultObjects = RE::BGSDefaultObjectManager::GetSingleton();
		const auto baseEnchantment = defaultObjects->GetObject<RE::EnchantmentItem>(
			a_isWeapon ? RE::DEFAULT_OBJECT::kBaseWeaponEnchantment
					   : RE::DEFAULT_OBJECT::kBaseArmorEnchantment);

		if (a_isWeapon && !a_effects.empty()) {
			const auto baseEffect = a_effects.front().baseEffect;
			if (baseEffect && baseEffect->data.delivery != RE::MagicSystem::Delivery::kTouch) {
				a_enchantment.SetCastingType(baseEffect->data.castingType);
				a_enchantment.SetDelivery(baseEffect->data.delivery);
				a_enchantment.data.spellType = RE::MagicSystem::SpellType::kStaffEnchantment;
				return;
			}
		}

		if (baseEnchantment) {
			a_enchantment.Copy(baseEnchantment);
		}
		else {
			a_enchantment.SetCastingType(
				a_isWeapon ? RE::MagicSystem::CastingType::kFireAndForget
						   : RE::MagicSystem::CastingType::kConstantEffect);
			a_enchantment.SetDelivery(
				a_isWeapon ? RE::MagicSystem::Delivery::kTouch : RE::MagicSystem::Delivery::kSelf);
		}
	}
}
