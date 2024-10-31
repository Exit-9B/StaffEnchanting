#pragma once

namespace JSONSettings
{
	class SettingsHolder
	{
	public:
		static SettingsHolder* GetSingleton()
		{
			static SettingsHolder singleton;
			return std::addressof(singleton);
		}

		void Read();
		[[nodiscard]] bool IsProhibitedSpell(const RE::SpellItem* a_spell) const;

		SettingsHolder(const SettingsHolder&) = delete;
		SettingsHolder(SettingsHolder&&) = delete;
		SettingsHolder& operator=(const SettingsHolder&) = delete;
		SettingsHolder& operator=(SettingsHolder&&) = delete;

	protected:
		SettingsHolder() = default;
		~SettingsHolder() = default;

	private:
		std::vector<RE::SpellItem*> excludedSpells;
	};
}
