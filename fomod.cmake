install(
	FILES
		"${DATA_DIR}/fomod/info.xml"
		"${DATA_DIR}/fomod/ModuleConfig.xml"
	DESTINATION "fomod"
	COMPONENT fomod
	EXCLUDE_FROM_ALL
)

install(
	FILES
		"${DATA_DIR}/fomod/Images/SkyrimSE.jpg"
		"${DATA_DIR}/fomod/Images/SkyrimVR.jpg"
		"${DATA_DIR}/fomod/Images/SkyUIRegular.jpg"
		"${DATA_DIR}/fomod/Images/SorcererPatch.jpg"
		"${DATA_DIR}/fomod/Images/StaffEnchanting.jpg"
	DESTINATION "fomod/Images"
	COMPONENT fomod
	EXCLUDE_FROM_ALL
)

install(
	FILES
		"${DATA_DIR}/Common/StaffEnchanting.esp"
	DESTINATION "Common"
	COMPONENT fomod
	EXCLUDE_FROM_ALL
)

install(
	FILES ${TRANSLATION_FILES}
	DESTINATION "Common/Interface/Translations"
	COMPONENT fomod
	EXCLUDE_FROM_ALL
)

install(
	FILES
		"${DATA_DIR}/Menu/SkyUI/Interface/StaffCraftingMenu.swf"
	DESTINATION "Menu/SkyUI/Interface"
	COMPONENT fomod
	EXCLUDE_FROM_ALL
)

install(
	FILES
		"${DATA_DIR}/Menu/SkyUI-VR/Interface/StaffCraftingMenu.swf"
	DESTINATION "Menu/SkyUI-VR/Interface"
	COMPONENT fomod
	EXCLUDE_FROM_ALL
)

install(
	FILES
		"${DATA_DIR}/Patches/Sorcerer/StaffEnchantingSorcererPatch.esp"
	DESTINATION "Patches/Sorcerer"
	COMPONENT fomod
	EXCLUDE_FROM_ALL
)
