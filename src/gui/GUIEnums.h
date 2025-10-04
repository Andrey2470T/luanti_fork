#pragma once

#include "Types.h"

enum class GUIElementType : u8
{
    Button,
    CheckBox,
    ComboBox,
    ContextMenu,
    Menu,
    EditBox,
    FileOpenDialog,
    ColorSelectDialog,
    InOutFader,
    Image,
    ListBox,
    MeshViewer,
    MessageBox,
    ModalScreen,
    ScrollBar,
    SpinBox,
    StaticText,
    Tab,
    TabControl,
    Table,
    ToolBar,
    TreeView,
    Window,
    Element,
    Root,
    Count
};

#define EGUI_ELEMENT_TYPE GUIElementType

#define EGUIET_BUTTON GUIElementType::Button
#define EGUIET_CHECK_BOX GUIElementType::CheckBox
#define EGUIET_COMBO_BOX GUIElementType::ComboBox
#define EGUIET_MENU GUIElementType::Menu
#define EGUIET_EDIT_BOX GUIElementType::EditBox
#define EGUIET_FILE_OPEN_DIALOG GUIElementType::FileOpenDialog
#define EGUIET_IMAGE GUIElementType::Image
#define EGUIET_LIST_BOX GUIElementType::ListBox
#define EGUIET_SCROLL_BAR GUIElementType::ScrollBar
#define EGUIET_STATIC_TEXT GUIElementType::StaticText
#define EGUIET_TAB GUIElementType::Tab
#define EGUIET_TAB_CONTROL GUIElementType::TabControl
#define EGUIET_TABLE GUIElementType::Table
#define EGUIET_TOOL_BAR GUIElementType::ToolBar
#define EGUIET_ELEMENT GUIElementType::Element
#define EGUIET_ROOT GUIElementType::Root
#define EGUIET_COUNT GUIElementType::Count


enum class GUIAlignment : u8
{
    UpperLeft,
    LowerRight,
    Center,
    Scale
};

#define EGUI_ALIGNMENT GUIAlignment

#define EGUIA_UPPERLEFT GUIAlignment::UpperLeft
#define EGUIA_LOWERRIGHT GUIAlignment::LowerRight
#define EGUIA_CENTER GUIAlignment::Center
#define EGUIA_SCALE GUIAlignment::Scale

enum class GUIFocusFlag : u8
{
	//! When set the focus changes when the left mouse-button got clicked while over an element
	SetOnLMouseDown = 0x1,

	//! When set the focus changes when the right mouse-button got clicked while over an element
	//! Note that elements usually don't care about right-click and that won't change with this flag
	//! This is mostly to allow taking away focus from elements with right-mouse additionally.
	SetOnRMouseDown = 0x2,

	//! When set the focus changes when the mouse-cursor is over an element
	SetOnMouseOver = 0x4,

	//! When set the focus can be changed with TAB-key combinations.
    SetOnTab = 0x8,

	//! When set it's possible to set the focus to disabled elements.
	CanFocusDisabled = 0x16
};

#define EFOCUS_FLAG GUIFocusFlag

#define EFF_SET_ON_LMOUSE_DOWN GUIFocusFlag::SetOnLMouseDown
#define EFF_SET_ON_RMOUSE_DOWN GUIFocusFlag::SetOnRMouseDown
#define EFF_SET_ON_MOUSE_OVER GUIFocusFlag::SetOnMouseOver
#define EFF_SET_ON_TAB GUIFocusFlag::SetOnTab
#define EFF_CAN_FOCUS_DISABLED GUIFocusFlag::CanFocusDisabled

//! Current state of buttons used for drawing sprites.
//! Note that up to 3 states can be active at the same time:
//! EGBS_BUTTON_UP or EGBS_BUTTON_DOWN
//! EGBS_BUTTON_MOUSE_OVER or EGBS_BUTTON_MOUSE_OFF
//! EGBS_BUTTON_FOCUSED or EGBS_BUTTON_NOT_FOCUSED
enum class GUIButtonState : u8
{
	//! The button is not pressed.
    Up = 0,
	//! The button is currently pressed down.
    Down,
	//! The mouse cursor is over the button
    MouseOver,
	//! The mouse cursor is not over the button
    MouseOff,
	//! The button has the focus
    Focused,
	//! The button doesn't have the focus
    NotFocused,
	//! The button is disabled All other states are ignored in that case.
    Disabled,
	//! not used, counts the number of enumerated items
    Count
};

#define EGUI_BUTTON_STATE GUIButtonState

#define EGBS_BUTTON_UP GUIButtonState::Up
#define EGBS_BUTTON_DOWN GUIButtonState::Down
#define EGBS_BUTTON_MOUSE_OVER GUIButtonState::MouseOver
#define EGBS_BUTTON_MOUSE_OFF GUIButtonState::MouseOff
#define EGBS_BUTTON_FOCUSED GUIButtonState::Focused
#define EGBS_BUTTON_NOT_FOCUSED GUIButtonState::NotFocused
#define EGBS_BUTTON_DISABLED GUIButtonState::Disabled
#define EGBS_COUNT GUIButtonState::Count


//! State of buttons used for drawing texture images.
//! Note that only a single state is active at a time
//! Also when no image is defined for a state it will use images from another state
//! and if that state is not set from the replacement for that,etc.
//! So in many cases setting EGBIS_IMAGE_UP and EGBIS_IMAGE_DOWN is sufficient.
enum class GUIButtonImageState : u8
{
	//! When no other states have images they will all use this one.
    Up,
	//! When not set EGBIS_IMAGE_UP is used.
    UpMouseOver,
	//! When not set EGBIS_IMAGE_UP_MOUSEOVER is used.
    UpFocused,
	//! When not set EGBIS_IMAGE_UP_FOCUSED is used.
    UpFocusedMouseOver,
	//! When not set EGBIS_IMAGE_UP is used.
    Down,
	//! When not set EGBIS_IMAGE_DOWN is used.
    DownMouseOver,
	//! When not set EGBIS_IMAGE_DOWN_MOUSEOVER is used.
    DownFocused,
	//! When not set EGBIS_IMAGE_DOWN_FOCUSED is used.
    DownFocusedMouseOver,
	//! When not set EGBIS_IMAGE_UP or EGBIS_IMAGE_DOWN are used (depending on button state).
    Disabled,
	//! not used, counts the number of enumerated items
    Count
};

#define EGUI_BUTTON_IMAGE_STATE GUIButtonImageState

#define EGBIS_IMAGE_UP GUIButtonImageState::Up
#define EGBIS_IMAGE_UP_MOUSEOVER GUIButtonImageState::UpMouseOver
#define EGBIS_IMAGE_UP_FOCUSED GUIButtonImageState::UpFocused
#define EGBIS_IMAGE_UP_FOCUSED_MOUSEOVER GUIButtonImageState::UpFocusedMouseOver
#define EGBIS_IMAGE_DOWN GUIButtonImageState::Down
#define EGBIS_IMAGE_DOWN_MOUSEOVER GUIButtonImageState::DownMouseOver
#define EGBIS_IMAGE_DOWN_FOCUSED GUIButtonImageState::DownFocused
#define EGBIS_IMAGE_DOWN_FOCUSED_MOUSEOVER GUIButtonImageState::DownFocusedMouseOver
#define EGBIS_IMAGE_DISABLED GUIButtonImageState::Disabled
#define EGBIS_COUNT GUIButtonImageState::Count

//! Enumeration for listbox colors
enum class GUIListBoxColor : u8
{
	//! Color of text
    Text = 0,
	//! Color of selected text
    TextHighlight,
	//! Color of icon
    Icon,
	//! Color of selected icon
    IconHighlight,
	//! Not used, just counts the number of available colors
    c
};

#define EGUI_LISTBOX_COLOR GUIListBoxColor

#define EGUI_LBC_TEXT GUIListBoxColor::Text
#define EGUI_LBC_TEXT_HIGHLIGHT GUIListBoxColor::TextHighlight
#define EGUI_LBC_ICON GUIListBoxColor::Icon
#define EGUI_LBC_ICON_HIGHLIGHT GUIListBoxColor::IconHighlight
#define EGUI_LBC_COUNT GUIListBoxColor::IconHighlight
