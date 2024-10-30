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
		bool IsProhibitedSpell(const RE::SpellItem* a_spell);

	protected:
		SettingsHolder() = default;
		~SettingsHolder() = default;

		SettingsHolder(const SettingsHolder&) = delete;
		SettingsHolder(SettingsHolder&&) = delete;
		SettingsHolder& operator=(const SettingsHolder&) = delete;
		SettingsHolder& operator=(SettingsHolder&&) = delete;

	private:
		std::vector<RE::SpellItem*> excludedSpells;
	};
}
