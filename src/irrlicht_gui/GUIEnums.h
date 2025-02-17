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
    Count,
    Force32Bit
};

enum class GUIAlignment : u8
{
    UpperLeft,
    LowerRight,
    Center,
    Scale
};

enum class FocusFlag : u8
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
