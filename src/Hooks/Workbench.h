#pragma once

namespace Hooks
{
	class Workbench
	{
	public:
		static void Install();

	private:
		static bool CheckFurniture(RE::TESObjectREFR* a_refr);

		inline static REL::Relocation<decltype(&CheckFurniture)> _IsFurniture;
	};
}
