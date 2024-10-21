#pragma once

namespace Hooks
{
	class Create
	{
	public:
		static void Install();

	private:
		static void InitEnchantment(
			RE::EnchantmentItem& a_enchantment,
			const RE::BSTArray<RE::Effect>& a_effects,
			bool a_isWeapon);
	};
}
