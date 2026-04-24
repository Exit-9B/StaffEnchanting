#include "AddWorn.h"
#include "RE/Offset.h"

namespace Hooks
{
	void AddWorn::Install()
	{
		REL::Relocation<std::byte*> hook{ STATIC_OFFSET(TESObjectWEAP::IsThrownWeapon) };
		REL::make_pattern<"0F B6 91 9D 01 00 00">().match_or_fail(hook.address());
		REL::safe_fill(hook.address(), REL::NOP, 0x18);
	}
}
