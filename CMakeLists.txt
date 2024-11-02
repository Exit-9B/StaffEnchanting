cmake_minimum_required(VERSION 3.24)

project(
	StaffEnchanting
	VERSION 1.1.0
	LANGUAGES CXX
)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

list(APPEND CMAKE_IGNORE_PATH "${PROJECT_BINARY_DIR}")
list(APPEND CMAKE_MODULE_PATH
	"${PROJECT_SOURCE_DIR}/tools/SKSE-CMakeModules/Modules"
)
include(SKSEPlugin)

set(INSTALL_SKYUI_MENU ON CACHE BOOL "Install custom menu for SkyUI.")

set(SKSE_USE_XBYAK ON)
set(SKSE_SUPPORT_VR ON)

SKSEPlugin_Add(
	${PROJECT_NAME}
	SOURCE_DIR src
	SOURCES
		.clang-format
		.editorconfig
		README.md
		vcpkg.json
	PRECOMPILE_HEADERS
		src/common/PCH.h
)

find_path(SIMPLEINI_INCLUDE_DIRS "SimpleIni.h")
find_package(jsoncpp CONFIG REQUIRED)

target_include_directories(
	${PROJECT_NAME}
	PRIVATE
		${SIMPLEINI_INCLUDE_DIRS}
)

target_link_libraries(
	${PROJECT_NAME}
	PRIVATE
		JsonCpp::JsonCpp
)

set_target_properties(CommonLibSSE PROPERTIES
	FOLDER External
)

set(DATA_DIR "${PROJECT_SOURCE_DIR}/data")

set(TRANSLATION_FILES
	"${DATA_DIR}/Common/Interface/Translations/StaffEnchanting_ENGLISH.txt"
)

install(
	FILES
		"${DATA_DIR}/Common/StaffEnchanting.esp"
	DESTINATION "."
)

install(
	FILES ${TRANSLATION_FILES}
	DESTINATION "Interface/Translations"
)

install(
	FILES "${DATA_DIR}/Common/SKSE/Plugins/StaffEnchanting.ini"
	DESTINATION "SKSE/Plugins"
)

install(
	FILES "${DATA_DIR}/Common/SKSE/Plugins/StaffEnchanting/StaffEnchanting_Vampirism.json"
	DESTINATION "SKSE/Plugins/StaffEnchanting"
)

if(INSTALL_SKYUI_MENU)
	install(
		FILES
			"${DATA_DIR}/Menu/SkyUI/Interface/StaffCraftingMenu.swf"
		DESTINATION "Interface"
	)
endif()

include(fomod.cmake)
