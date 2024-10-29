cmake_minimum_required(VERSION 3.24)

project(
	StaffEnchanting
	VERSION 0.0.0
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

set_target_properties(CommonLibSSE PROPERTIES
	FOLDER External
)

set(COMMON_DATA_DIR "${PROJECT_SOURCE_DIR}/data/Common")
if(NOT BUILD_SKYRIMVR)
	set(GAME_DATA_DIR "${PROJECT_SOURCE_DIR}/data/SkyrimSE")
else()
	set(GAME_DATA_DIR "${PROJECT_SOURCE_DIR}/data/SkyrimVR")
endif()

install(
	FILES
		"${COMMON_DATA_DIR}/StaffEnchanting.esp"
	DESTINATION
		"."
)

install(
	FILES
		"${COMMON_DATA_DIR}/Interface/Translations/StaffEnchanting_ENGLISH.txt"
	DESTINATION
		"Interface/Translations"
)

if(INSTALL_SKYUI_MENU AND NOT BUILD_SKYRIMVR)
	install(
		FILES
			"${GAME_DATA_DIR}/Interface/StaffCraftingMenu.swf"
		DESTINATION
			"Interface"
	)
endif()

set(CPACK_GENERATOR "7Z")
set(CPACK_PACKAGE_FILE_NAME ${PROJECT_NAME}-${PROJECT_VERSION})
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY FALSE)
include(CPack)
