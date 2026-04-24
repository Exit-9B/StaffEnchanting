#include "AddWorn.h"
#include "RE/Offset.h"

namespace Hooks
{
	void AddWorn::Install()
	{
		auto hook = util::GameAddress(RE::Offset::TESObjectWEAP::IsThrownWeapon);
		REL::safe_fill(hook.address(), REL::NOP, 0x18);
	}
}
