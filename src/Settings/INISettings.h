#pragma once

class INISettings
{
public:
	INISettings();

	struct Settings_StaffEnchanting
	{
		bool bAllowSelfTargeted = false;
		bool bUseSpellTomes = false;
	};

	[[nodiscard]] constexpr const auto& StaffEnchanting() const { return StaffEnchanting_; }

private:
	Settings_StaffEnchanting StaffEnchanting_;
};
