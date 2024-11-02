#include "BaseCharge.h"
#include "RE/Offset.h"

#include <xbyak/xbyak.h>

namespace Hooks
{
	void BaseCharge::Install()
	{
		auto hook = util::GameAddress(RE::Offset::TESEnchantableForm::GetFormBaseCharge, 0x2D);
		REL::make_pattern<"0F B7 58 12 66 85 DB">().match_or_fail(hook.address());

		REL::safe_fill(hook.address(), REL::NOP, 0x7);

		struct Patch : Xbyak::CodeGenerator
		{
			Patch(const std::byte* a_retnAddr)
				: Xbyak::CodeGenerator(21, SKSE::GetTrampoline().allocate(21))
			{
				Xbyak::Label retnLbl;

				mov(rbx, ptr[rax + offsetof(RE::TESEnchantableForm, formEnchanting)]);
				test(rbx, rbx);
				jz(retnLbl, T_SHORT);
				movzx(ebx, word[rax + offsetof(RE::TESEnchantableForm, amountofEnchantment)]);
				test(bx, bx);
				L(retnLbl);
				jmp(a_retnAddr);
			}
		} patch{ hook.get() + 0x7 };

		SKSE::GetTrampoline().write_branch<5>(hook.address(), patch.getCode());
	}
}
