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

	static std::vector<std::string> Split(const std::string& input)
	{
		std::vector<std::string> parts;
		size_t start = 0;
		size_t end = input.find('|');

		while (end != std::string::npos) {
			parts.push_back(input.substr(start, end - start));
			start = end + 1;
			end = input.find('|', start);
		}

		parts.push_back(input.substr(start));

		return parts;
	}

	static bool IsHex(std::string const& s)
	{
		return s.compare(0, 2, "0x") == 0 && s.size() > 2 &&
			s.find_first_not_of("0123456789abcdefABCDEF", 2) == std::string::npos;
	}

	static RE::FormID GetSpellFormID(const std::string& a_identifier)
	{
		if (const auto splitID = Split(a_identifier); splitID.size() == 2) {
			if (!IsHex(splitID[1]))
				return 0;
			const RE::FormID localFormID = std::stoi(splitID[1], nullptr, 16);

			const auto& modName = splitID[0];
			if (!RE::TESDataHandler::GetSingleton()->LookupModByName(modName))
				return 0;

			auto* baseForm = RE::TESDataHandler::GetSingleton()->LookupForm<RE::SpellItem>(
				localFormID,
				modName);
			return baseForm ? baseForm->formID : 0;
		}
		return 0;
	}

	void Read()
	{
		std::vector<std::string> paths{};
		try {
			paths = findJsonFiles();
		}
		catch (std::exception e) {
			logger::warn("Caught {} while reading files.", e.what());
			return;
		}
		if (paths.empty()) {
			return;
		}

		for (const auto& path : paths) {
			Json::Reader JSONReader;
			Json::Value JSONFile;
			try {
				std::ifstream rawJSON(path);
				JSONReader.parse(rawJSON, JSONFile);
			}
			catch (Json::Exception e) {
				logger::warn("Caught {} while reading files.", e.what());
				continue;
			}
			catch (std::exception e) {
				logger::error("Caught unhandled exception {} while reading files.", e.what());
				continue;
			}

			if (!JSONFile.isObject()) {
				logger::warn("Warning: {} is not an object. File will be ignored.", path);
				continue;
			}

			const auto& exclusions = JSONFile["exclusions"];
			if (!exclusions || !exclusions.isArray()) {
				logger::warn(
					"Warning: {} is missing exclusions, or it is not an array. File will be "
					"ignored.", path);
				continue;
			}

			for (const auto& entry : JSONFile) {
				if (const auto& entryText = entry.isString() ? entry.asString() : "";
					!entryText.empty()) {
					const auto FoundSpellID = GetSpellFormID(entryText);
					if (FoundSpellID == 0) {
						logger::warn("Failed to find spell {} in {}.", entryText, path);
						continue;
					}

					UI::StaffCraftingMenu::AddExcludedSpell(FoundSpellID);
				}
			}
		}
	}
}
