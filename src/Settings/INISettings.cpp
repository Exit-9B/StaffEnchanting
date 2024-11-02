#include "INISettings.h"

#include <SimpleIni.h>

INISettings::INISettings()
{
	::CSimpleIniA ini{};
	ini.SetUnicode();
	ini.LoadFile(fmt::format(R"(.\Data\SKSE\Plugins\{}.ini)", Plugin::NAME).c_str());

	StaffEnchanting_
		.bAllowSelfTargeted = ini.GetBoolValue("StaffEnchanting", "bAllowSelfTargeted", false);
	StaffEnchanting_.bUseSpellTomes = ini.GetBoolValue("StaffEnchanting", "bUseSpellTomes", false);
}
