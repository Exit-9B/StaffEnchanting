#include "BaseCharge.h"
#include "RE/Offset.h"

#include <xbyak/xbyak.h>

namespace Hooks
{
	void BaseCharge::Install()
	{
		static auto hook = REL::Relocation<std::byte*>(
			RE::Offset::TESEnchantableForm::GetFormBaseCharge,
			0x2D);

		REL::make_pattern<"0F B7 58 12 66 85 DB">().match_or_fail(hook.address());

		REL::safe_fill(hook.address(), REL::NOP, 0x7);

		struct Patch : Xbyak::CodeGenerator
		{
			Patch() : Xbyak::CodeGenerator(21, SKSE::GetTrampoline().allocate(21))
			{
				Xbyak::Label retn;

				mov(rbx, ptr[rax + offsetof(RE::TESEnchantableForm, formEnchanting)]);
				test(rbx, rbx);
				jz(retn, Xbyak::CodeGenerator::T_SHORT);
				movzx(ebx, word[rax + offsetof(RE::TESEnchantableForm, amountofEnchantment)]);
				test(bx, bx);
				L(retn);
				jmp(hook.get() + 0x7);
			}
		} patch{};

		SKSE::GetTrampoline().write_branch<5>(hook.address(), patch.getCode());
	}
}
