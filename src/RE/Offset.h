#pragma once

namespace RE
{
	namespace Offset
	{
		namespace BGSCreatedObjectManager
		{
#ifdef SKYRIMVR
			constexpr auto InitEnchantment = REL::Offset(0x5A7050);
#endif
		}

		namespace MagicItemCreationHelpers
		{
			constexpr auto CreateNewEnchantment = MAKE_OFFSET(36178, 0x5A7E20);
		}

		namespace TESEnchantableForm
		{
			constexpr auto GetFormBaseCharge = MAKE_OFFSET(14564, 0x1A0AF0);
		}

		namespace TESObjectREFR
		{
			constexpr auto ActivateCraftingWorkbench = MAKE_OFFSET(52941, 0x90B2D0);
		}
	}
}
