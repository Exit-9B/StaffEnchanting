#include "JSONSettings.h"

#include "UI/StaffCraftingMenu.h"
#include "json/json.h"

namespace JSONSettings
{
	static std::vector<std::string> findJsonFiles()
	{
		static constexpr std::string_view directory = R"(Data/SKSE/Plugins/StaffEnchanting)";
		std::vector<std::string> jsonFilePaths;
		for (const auto& entry : std::filesystem::directory_iterator(directory)) {
			if (entry.is_regular_file() && entry.path().extension() == ".json") {
				jsonFilePaths.push_back(entry.path().string());
			}
		}

		std::sort(jsonFilePaths.begin(), jsonFilePaths.end());
		return jsonFilePaths;
	}

	static RE::SpellItem* GetSpellFormID(const std::string& a_identifier)
	{
		std::istringstream ss{ a_identifier };
		std::string plugin, id;

		std::getline(ss, plugin, '|');
		std::getline(ss, id);
		RE::FormID rawFormID;
		std::istringstream(id) >> std::hex >> rawFormID;

		const auto dataHandler = RE::TESDataHandler::GetSingleton();
		return dataHandler->LookupForm<RE::SpellItem>(rawFormID, plugin);
	}

	void SettingsHolder::Read()
	{
		std::vector<std::string> paths{};
		try {
			paths = findJsonFiles();
		}
		catch (const std::exception& e) {
			logger::warn("Caught {} while reading files.", e.what());
			return;
		}
		if (paths.empty()) {
			logger::info("No settings found");
			return;
		}

		for (const auto& path : paths) {
			Json::Reader JSONReader;
			Json::Value JSONFile;
			try {
				std::ifstream rawJSON(path);
				JSONReader.parse(rawJSON, JSONFile);
			}
			catch (const Json::Exception& e) {
				logger::warn("Caught {} while reading files.", e.what());
				continue;
			}
			catch (const std::exception& e) {
				logger::error("Caught unhandled exception {} while reading files.", e.what());
				continue;
			}

			if (!JSONFile.isObject()) {
				logger::warn("Warning: <{}> is not an object. File will be ignored.", path);
				continue;
			}

			const auto& exclusions = JSONFile["exclusions"];
			const auto& whitelists = JSONFile["whitelist"];
			if (exclusions && !exclusions.isArray()) {
				logger::warn(
					"Warning: <{}> has an incorrectly formatted exclusions field, file will be "
					"ignored.",
					path);
				continue;
			}

			if (whitelists && !whitelists.isArray()) {
				logger::warn(
					"Warning: <{}> has an incorrectly formatted whitelist, file will be ignored.",
					path);
				continue;
			}

			if (!exclusions && !whitelists) {
				logger::warn(
					"Warning: <{}> is missing both exclusions and whitelist, file will be "
					"ingored.",
					path);
				continue;
			}

			if (exclusions) {
				for (const auto& entry : exclusions) {
					if (const auto& entryText = entry.isString() ? entry.asString() : "";
						!entryText.empty()) {
						const auto foundSpell = GetSpellFormID(entryText);
						if (!foundSpell) {
							logger::warn("Failed to find spell <{}> in <{}>.", entryText, path);
							continue;
						}

						const auto it = std::ranges::lower_bound(excludedSpells, foundSpell);
						if (it == std::ranges::end(excludedSpells) || *it != foundSpell) {
							excludedSpells.emplace(it, foundSpell);
						}
					}
				}
			}

			if (whitelists) {
				for (const auto& entry : whitelists) {
					if (const auto& entryText = entry.isString() ? entry.asString() : "";
						!entryText.empty()) {
						const auto foundSpell = GetSpellFormID(entryText);
						if (!foundSpell) {
							logger::warn("Failed to find spell <{}> in <{}>.", entryText, path);
							continue;
						}

						const auto it = std::ranges::lower_bound(whitelistedSpells, foundSpell);
						if (it == std::ranges::end(whitelistedSpells) || *it != foundSpell) {
							whitelistedSpells.emplace(it, foundSpell);
						}
					}
				}
			}
		}
	}

	bool SettingsHolder::IsProhibitedSpell(const RE::SpellItem* a_spell) const
	{
		const auto it = std::ranges::lower_bound(excludedSpells, a_spell);
		return it != std::ranges::end(excludedSpells) && *it == a_spell;
	}

	bool SettingsHolder::IsWhitelistedSpell(const RE::SpellItem* a_spell) const
	{
		const auto it = std::ranges::lower_bound(whitelistedSpells, a_spell);
		return it != std::ranges::end(whitelistedSpells) && *it == a_spell;
	}
}
