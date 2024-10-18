#pragma once

namespace Hooks
{
	class Workbench
	{
	public:
		static void Install();
		/*
		 * Returns the workbench parameters as a pair of bools.
		 * @return Pair of bools, representing [disallowHeartStones, allowSoulGems].
		 */
		static std::pair<bool, bool> GetWorkbenchType();

	private:
		static bool CheckFurniture(RE::TESObjectREFR* a_refr);

		inline static REL::Relocation<decltype(&CheckFurniture)> _IsFurniture;
		inline static bool _disallowHeartStones;
		inline static bool _allowSoulGems;
	};
}
