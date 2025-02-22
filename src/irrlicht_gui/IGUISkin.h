// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#pragma once

#include "GUIEnums.h"
#include "Image/Color.h"
#include "Utils/Rect.h"

class IGUIFont;
class IGUISpriteBank;
class IGUIElement;

//! Enumeration of available default skins.
/** To set one of the skins, use the following code, for example to set
the Windows classic skin:
\code
gui::IGUISkin* newskin = environment->createSkin(gui::EGST_WINDOWS_CLASSIC);
environment->setSkin(newskin);
newskin->drop();
\endcode
*/
enum GUISkinType : u8
{
	//! Default windows look and feel
    WindowsClassic = 0,

	//! Like EGST_WINDOWS_CLASSIC, but with metallic shaded windows and buttons
    WindowsMetallic,

	//! Burning's skin
    BurningSkin,

	//! An unknown skin, not serializable at present
    Unknown,

	//! this value is not used, it only specifies the number of skin types
    Count
};

//! Enumeration for skin colors
enum class GUIDefaultColor
{
	//! Dark shadow for three-dimensional display elements.
    DarkShadows3D = 0,
	//! Shadow color for three-dimensional display elements (for edges facing away from the light source).
    Shadow3D,
	//! Face color for three-dimensional display elements and for dialog box backgrounds.
    Face3D,
	//! Highlight color for three-dimensional display elements (for edges facing the light source.)
    HighLight3D,
	//! Light color for three-dimensional display elements (for edges facing the light source.)
    Light3D,
	//! Active window border.
    ActiveBorder,
	//! Active window title bar text.
    ActiveCaption,
	//! Background color of multiple document interface (MDI) applications.
    AppWorkspace,
	//! Text on a button
    ButtonText,
	//! Grayed (disabled) text.
    GrayText,
	//! Item(s) selected in a control.
    HighLight,
	//! Text of item(s) selected in a control.
    HighLightText,
	//! Inactive window border.
    InactiveBorder,
	//! Inactive window caption.
    InactiveCaption,
	//! Tool tip text color
    Tooltip,
	//! Tool tip background color
    TooltipBackground,
	//! Scrollbar gray area
    Scrollbar,
	//! Window background
    Window,
	//! Window symbols like on close buttons, scroll bars and check boxes
    WindowSymbol,
	//! Icons in a list or tree
    Icon,
	//! Selected icons in a list or tree
    IconHighLight,
	//! Grayed (disabled) window symbols like on close buttons, scroll bars and check boxes
    GrayWindowSymbol,
	//! Window background for editable field (editbox, checkbox-field)
    Editable,
	//! Grayed (disabled) window background for editable field (editbox, checkbox-field)
    GrayEditable,
	//! Show focus of window background for editable field (editbox or when checkbox-field is pressed)
    FocusedEditable,

	//! this value is not used, it only specifies the amount of default colors
	//! available.
    Count
};

//! Enumeration for default sizes.
enum class GUIDefaultSize
{
	//! default with / height of scrollbar. Also width of drop-down button in comboboxes.
    ScrollbarSize = 0,
	//! height of menu
    MenuHeight,
	//! width and height of a window titlebar button (like minimize/maximize/close buttons). The titlebar height is also calculated from that.
    WindowButtonWidth,
	//! width of a checkbox check
    CheckBoxWidth,
	//! \deprecated This may be removed by Irrlicht 1.9
    MessageBoxWidth,
	//! \deprecated This may be removed by Irrlicht 1.9
    MessageBoxHeight,
	//! width of a default button
    ButtonWidth,
	//! height of a default button (OK and cancel buttons)
    ButtonHeight,
	//! distance for text from background
    TextDistanceX,
	//! distance for text from background
    TextDistanceY,
	//! distance for text in the title bar, from the left of the window rect
    TitlebartextDistanceX,
	//! distance for text in the title bar, from the top of the window rect
    TitlebartextDistanceY,
	//! free space in a messagebox between borders and contents on all sides
    MessageBoxGapSpace,
	//! minimal space to reserve for messagebox text-width
    MessageBoxMinTextWidth,
	//! maximal space to reserve for messagebox text-width
    MessageBoxMaxTextWidth,
	//! minimal space to reserve for messagebox text-height
    MessageBoxMinTextHeight,
	//! maximal space to reserve for messagebox text-height
    MessageBoxMaxTextHeight,
	//! pixels to move an unscaled button image to the right when a button is pressed and the unpressed image looks identical
    ButtonPressedImageOffsetX,
	//! pixels to move an unscaled button image down when a button is pressed  and the unpressed image looks identical
    ButtonPressedImageOffsetY,
	//! pixels to move the button text to the right when a button is pressed
    ButtonPressedTextOffsetX,
	//! pixels to move the button text down when a button is pressed
    ButtonPressedTextOffsetY,
	//! pixels to move an unscaled button sprite to the right when a button is pressed
    ButtonPressedSpriteOffsetX,
	//! pixels to move an unscaled button sprite down when a button is pressed
    ButtonPressedSpriteOffsetY,

	//! this value is not used, it only specifies the amount of default sizes
	//! available.
    Count
};

enum class GUIDefaultText : u8
{
	//! Text for the OK button on a message box
    MsgBoxOk = 0,
	//! Text for the Cancel button on a message box
    MsgBoxCancel,
	//! Text for the Yes button on a message box
    MsgBoxYes,
	//! Text for the No button on a message box
    MsgBoxNo,
	//! Tooltip text for window close button
    WindowClose,
	//! Tooltip text for window maximize button
    WindowMaximize,
	//! Tooltip text for window minimize button
    WindowMinimize,
	//! Tooltip text for window restore button
    WindowRestore,

	//! this value is not used, it only specifies the number of default texts
    Count
};

//! Customizable symbols for GUI
enum class GUIDefaultIcon
{
	//! maximize window button
    WindowMaximize = 0,
	//! restore window button
    WindowRestore,
	//! close window button
    WindowClose,
	//! minimize window button
    WindowMinimize,
	//! resize icon for bottom right corner of a window
    WindowResize,
	//! scroll bar up button
    CursorUp,
	//! scroll bar down button
    CursorDown,
	//! scroll bar left button
    CursorLeft,
	//! scroll bar right button
    CursorRight,
	//! icon for menu children
    MenuMore,
	//! tick for checkbox
    CheckBoxChecked,
	//! down arrow for dropdown menus
    Dropdown,
	//! smaller up arrow
    SmallCursorUp,
	//! smaller down arrow
    SmallCursorDown,
	//! selection dot in a radio button
    RadioButtonChecked,
	//! << icon indicating there is more content to the left
    MoreLeft,
	//! >> icon indicating that there is more content to the right
    MoreRight,
	//! icon indicating that there is more content above
    MoreUp,
	//! icon indicating that there is more content below
    MoreDown,
	//! plus icon for trees
    Expand,

	//! minus icon for trees
    Collapse,
	//! file icon for file selection
    File,
	//! folder icon for file selection
    Directory,

	//! value not used, it only specifies the number of icons
    Count
};

// Customizable fonts
enum class GUIDefaultFont : u8
{
	//! For static text, edit boxes, lists and most other places
    Default = 0,
	//! Font for buttons
    Button,
	//! Font for window title bars
    Window,
	//! Font for menu items
    Menu,
	//! Font for tooltips
    Tooltip,
	//! this value is not used, it only specifies the amount of default fonts
	//! available.
    Count
};

//! A skin modifies the look of the GUI elements.
class IGUISkin
{
public:
	//! returns display density scaling factor
	virtual float getScale() const = 0;

	//! sets display density scaling factor
	virtual void setScale(float scale) = 0;

	//! returns default color
    virtual img::color8 getColor(GUIDefaultColor color) const = 0;

	//! sets a default color
    virtual void setColor(GUIDefaultColor which, img::color8 newColor) = 0;

	//! returns size for the given size type
    virtual s32 getSize(GUIDefaultSize size) const = 0;

	//! Returns a default text.
	/** For example for Message box button captions:
	"OK", "Cancel", "Yes", "No" and so on. */
    virtual const wchar_t *getDefaultText(GUIDefaultText text) const = 0;

	//! Sets a default text.
	/** For example for Message box button captions:
	"OK", "Cancel", "Yes", "No" and so on. */
    virtual void setDefaultText(GUIDefaultText which, const wchar_t *newText) = 0;

	//! sets a default size
    virtual void setSize(GUIDefaultSize which, s32 size) = 0;

	//! returns the default font
    virtual IGUIFont *getFont(GUIDefaultFont which = GUIDefaultFont::Default) const = 0;

	//! sets a default font
    virtual void setFont(IGUIFont *font, GUIDefaultFont which = GUIDefaultFont::Default) = 0;

	//! returns the sprite bank
	virtual IGUISpriteBank *getSpriteBank() const = 0;

	//! sets the sprite bank
	virtual void setSpriteBank(IGUISpriteBank *bank) = 0;

	//! Returns a default icon
	/** Returns the sprite index within the sprite bank */
    virtual u32 getIcon(GUIDefaultIcon icon) const = 0;

	//! Sets a default icon
	/** Sets the sprite index used for drawing icons like arrows,
	close buttons and ticks in checkboxes
	\param icon: Enum specifying which icon to change
	\param index: The sprite index used to draw this icon */
    virtual void setIcon(GUIDefaultIcon icon, u32 index) = 0;

	//! draws a standard 3d button pane
	/** Used for drawing for example buttons in normal state.
	It uses the colors EGDC_3D_DARK_SHADOW, EGDC_3D_HIGH_LIGHT, EGDC_3D_SHADOW and
    EGDC_3D_FACE for this. See GUIDefaultColor for details.
	\param element: Pointer to the element which wishes to draw this. This parameter
	is usually not used by IGUISkin, but can be used for example by more complex
	implementations to find out how to draw the part exactly.
	\param rect: Defining area where to draw.
	\param clip: Clip area. */
    virtual void draw3DButtonPaneStandard(std::shared_ptr<IGUIElement> element,
            const recti &rect,
            const recti *clip = 0) = 0;
    virtual void drawColored3DButtonPaneStandard(std::shared_ptr<IGUIElement> element,
            const recti& rect,
            const recti* clip=0,
            const img::color8* colors=0) = 0;

	//! draws a pressed 3d button pane
	/** Used for drawing for example buttons in pressed state.
	It uses the colors EGDC_3D_DARK_SHADOW, EGDC_3D_HIGH_LIGHT, EGDC_3D_SHADOW and
    EGDC_3D_FACE for this. See GUIDefaultColor for details.
	\param element: Pointer to the element which wishes to draw this. This parameter
	is usually not used by IGUISkin, but can be used for example by more complex
	implementations to find out how to draw the part exactly.
	\param rect: Defining area where to draw.
	\param clip: Clip area. */
    virtual void draw3DButtonPanePressed(std::shared_ptr<IGUIElement> element,
            const recti &rect,
            const recti *clip = 0) = 0;
    virtual void drawColored3DButtonPanePressed(std::shared_ptr<IGUIElement> element,
            const recti& rect,
            const recti* clip=0,
            const img::color8* colors=0) = 0;

	//! draws a sunken 3d pane
	/** Used for drawing the background of edit, combo or check boxes.
	\param element: Pointer to the element which wishes to draw this. This parameter
	is usually not used by IGUISkin, but can be used for example by more complex
	implementations to find out how to draw the part exactly.
	\param bgcolor: Background color.
	\param flat: Specifies if the sunken pane should be flat or displayed as sunken
	deep into the ground.
	\param fillBackGround: Specifies if the background should be filled with the background
	color or not be drawn at all.
	\param rect: Defining area where to draw.
	\param clip: Clip area. */
    virtual void draw3DSunkenPane(std::shared_ptr<IGUIElement> element,
            img::color8 bgcolor, bool flat, bool fillBackGround,
            const recti &rect,
            const recti *clip = 0) = 0;

	//! draws a window background
	/** Used for drawing the background of dialogs and windows.
	\param element: Pointer to the element which wishes to draw this. This parameter
	is usually not used by IGUISkin, but can be used for example by more complex
	implementations to find out how to draw the part exactly.
	\param titleBarColor: Title color.
	\param drawTitleBar: True to enable title drawing.
	\param rect: Defining area where to draw.
	\param clip: Clip area.
	\param checkClientArea: When set to non-null the function will not draw anything,
	but will instead return the clientArea which can be used for drawing by the calling window.
	That is the area without borders and without titlebar.
	\return Returns rect where it would be good to draw title bar text. This will
	work even when checkClientArea is set to a non-null value.*/
    virtual recti draw3DWindowBackground(std::shared_ptr<IGUIElement> element,
            bool drawTitleBar, img::color8 titleBarColor,
            const recti &rect,
            const recti *clip = 0,
            recti *checkClientArea = 0) = 0;

	//! draws a standard 3d menu pane
	/** Used for drawing for menus and context menus.
	It uses the colors EGDC_3D_DARK_SHADOW, EGDC_3D_HIGH_LIGHT, EGDC_3D_SHADOW and
    EGDC_3D_FACE for this. See GUIDefaultColor for details.
	\param element: Pointer to the element which wishes to draw this. This parameter
	is usually not used by IGUISkin, but can be used for example by more complex
	implementations to find out how to draw the part exactly.
	\param rect: Defining area where to draw.
	\param clip: Clip area. */
    virtual void draw3DMenuPane(std::shared_ptr<IGUIElement> element,
            const recti &rect,
            const recti *clip = 0) = 0;

	//! draws a standard 3d tool bar
	/** Used for drawing for toolbars and menus.
	\param element: Pointer to the element which wishes to draw this. This parameter
	is usually not used by IGUISkin, but can be used for example by more complex
	implementations to find out how to draw the part exactly.
	\param rect: Defining area where to draw.
	\param clip: Clip area. */
    virtual void draw3DToolBar(std::shared_ptr<IGUIElement> element,
            const recti &rect,
            const recti *clip = 0) = 0;

	//! draws a tab button
	/** Used for drawing for tab buttons on top of tabs.
	\param element: Pointer to the element which wishes to draw this. This parameter
	is usually not used by IGUISkin, but can be used for example by more complex
	implementations to find out how to draw the part exactly.
	\param active: Specifies if the tab is currently active.
	\param rect: Defining area where to draw.
	\param clip: Clip area.
	\param alignment Alignment of GUI element. */
    virtual void draw3DTabButton(std::shared_ptr<IGUIElement> element, bool active,
            const recti &rect, const recti *clip = 0, GUIAlignment alignment = GUIAlignment::UpperLeft) = 0;

	//! draws a tab control body
	/** \param element: Pointer to the element which wishes to draw this. This parameter
	is usually not used by IGUISkin, but can be used for example by more complex
	implementations to find out how to draw the part exactly.
	\param border: Specifies if the border should be drawn.
	\param background: Specifies if the background should be drawn.
	\param rect: Defining area where to draw.
	\param clip: Clip area.
	\param tabHeight Height of tab.
	\param alignment Alignment of GUI element. */
    virtual void draw3DTabBody(std::shared_ptr<IGUIElement> element, bool border, bool background,
            const recti &rect, const recti *clip = 0, s32 tabHeight = -1, GUIAlignment alignment = GUIAlignment::UpperLeft) = 0;

	//! draws an icon, usually from the skin's sprite bank
	/** \param element: Pointer to the element which wishes to draw this icon.
	This parameter is usually not used by IGUISkin, but can be used for example
	by more complex implementations to find out how to draw the part exactly.
	\param icon: Specifies the icon to be drawn.
	\param position: The position to draw the icon
	\param starttime: The time at the start of the animation
	\param currenttime: The present time, used to calculate the frame number
	\param loop: Whether the animation should loop or not
	\param clip: Clip area. */
    virtual void drawIcon(std::shared_ptr<IGUIElement> element, GUIDefaultIcon icon,
            const v2i position, u32 starttime = 0, u32 currenttime = 0,
            bool loop = false, const recti *clip = 0) = 0;

	//! draws a 2d rectangle.
	/** \param element: Pointer to the element which wishes to draw this icon.
	This parameter is usually not used by IGUISkin, but can be used for example
	by more complex implementations to find out how to draw the part exactly.
	\param color: Color of the rectangle to draw. The alpha component specifies how
	transparent the rectangle will be.
	\param pos: Position of the rectangle.
	\param clip: Pointer to rectangle against which the rectangle will be clipped.
	If the pointer is null, no clipping will be performed. */
    virtual void draw2DRectangle(std::shared_ptr<IGUIElement> element, const img::color8 &color,
            const recti &pos, const recti *clip = 0) = 0;

	//! get the type of this skin
    virtual GUISkinType getType() const { return GUISkinType::Unknown; }
};
