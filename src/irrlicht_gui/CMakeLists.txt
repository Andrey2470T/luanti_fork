if (BUILD_CLIENT)
	set(irrlicht_gui_SRCS
		${CMAKE_CURRENT_SOURCE_DIR}/CGUIButton.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/CGUICheckBox.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/CGUIComboBox.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/CGUIEditBox.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/CGUIEnvironment.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/CGUIFileOpenDialog.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/CGUIFont.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/CGUIImage.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/CGUIListBox.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/CGUIScrollBar.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/GUISkin.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/CGUIStaticText.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/CGUITabControl.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/CGUISpriteBank.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/CGUIImageList.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/static_text.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/CGUITTFont.cpp
	)

	# CMake require us to set a local scope and then parent scope
	# Else the last set win in parent scope
	set(irrlicht_gui_SRCS ${irrlicht_gui_SRCS} PARENT_SCOPE)
endif()

