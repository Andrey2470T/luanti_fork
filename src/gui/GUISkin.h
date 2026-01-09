// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#pragma once

#include "GUIEnums.h"
#include "client/ui/extra_images.h"
#include "client/ui/sprite.h"
#include <Image/Color.h>
#include <Utils/Rect.h>
#include <Render/TTFont.h>

namespace gui
{
class IGUIElement;
class IGUISpriteBank;
}
class Renderer;

//! Enumeration of available default skins.
/** To set one of the skins, use the following code, for example to set
the Windows classic skin:
\code
gui::GUISkin* newskin = environment->createSkin(gui::EGST_WINDOWS_CLASSIC);
environment->setSkin(newskin);
newskin->drop();
\endcode
*/
enum class GUISkinType : u8
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
enum class GUIDefaultColor : u8
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

#define EGUI_DEFAULT_COLOR GUIDefaultColor

#define EGDC_3D_DARK_SHADOW GUIDefaultColor::DarkShadows3D
#define EGDC_3D_SHADOW GUIDefaultColor::Shadow3D
#define EGDC_3D_FACE GUIDefaultColor::Face3D
#define EGDC_3D_HIGH_LIGHT GUIDefaultColor::HighLight3D
#define EGDC_3D_LIGHT GUIDefaultColor::Light3D
#define EGDC_ACTIVE_BORDER GUIDefaultColor::ActiveBorder
#define EGDC_ACTIVE_CAPTION GUIDefaultColor::ActiveCaption
#define EGDC_APP_WORKSPACE GUIDefaultColor::AppWorkspace
#define EGDC_BUTTON_TEXT GUIDefaultColor::ButtonText
#define EGDC_GRAY_TEXT GUIDefaultColor::GrayText
#define EGDC_HIGH_LIGHT GUIDefaultColor::HighLight
#define EGDC_HIGH_LIGHT_TEXT GUIDefaultColor::HighLightText
#define EGDC_INACTIVE_BORDER GUIDefaultColor::InactiveBorder
#define EGDC_INACTIVE_CAPTION GUIDefaultColor::InactiveCaption
#define EGDC_TOOLTIP GUIDefaultColor::Tooltip
#define EGDC_TOOLTIP_BACKGROUND GUIDefaultColor::TooltipBackground
#define EGDC_SCROLLBAR GUIDefaultColor::Scrollbar
#define EGDC_WINDOW GUIDefaultColor::Window
#define EGDC_WINDOW_SYMBOL GUIDefaultColor::WindowSymbol
#define EGDC_ICON GUIDefaultColor::Icon
#define EGDC_ICON_HIGH_LIGHT GUIDefaultColor::IconHighLight
#define EGDC_GRAY_WINDOW_SYMBOL GUIDefaultColor::GrayWindowSymbol
#define EGDC_EDITABLE GUIDefaultColor::Editable
#define EGDC_GRAY_EDITABLE GUIDefaultColor::GrayEditable
#define EGDC_FOCUSED_EDITABLE GUIDefaultColor::FocusedEditable
#define EGDC_COUNT GUIDefaultColor::Count

//! Enumeration for default sizes.
enum class GUIDefaultSize : u8
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

#define EGUI_DEFAULT_SIZE GUIDefaultSize

#define EGDS_SCROLLBAR_SIZE GUIDefaultSize::ScrollbarSize
#define EGDS_MENU_HEIGHT GUIDefaultSize::MenuHeight
#define EGDS_WINDOW_BUTTON_WIDTH GUIDefaultSize::WindowButtonWidth
#define EGDS_CHECK_BOX_WIDTH GUIDefaultSize::CheckBoxWidth
#define EGDS_MESSAGE_BOX_WIDTH GUIDefaultSize::MessageBoxWidth
#define EGDS_MESSAGE_BOX_HEIGHT GUIDefaultSize::MessageBoxHeight
#define EGDS_BUTTON_WIDTH GUIDefaultSize::ButtonWidth
#define EGDS_BUTTON_HEIGHT GUIDefaultSize::ButtonHeight
#define EGDS_TEXT_DISTANCE_X GUIDefaultSize::TextDistanceX
#define EGDS_TEXT_DISTANCE_Y GUIDefaultSize::TextDistanceY
#define EGDS_TITLEBARTEXT_DISTANCE_X GUIDefaultSize::TitlebartextDistanceX
#define EGDS_TITLEBARTEXT_DISTANCE_Y GUIDefaultSize::TitlebartextDistanceY
#define EGDS_MESSAGE_BOX_GAP_SPACE GUIDefaultSize::MessageBoxGapSpace
#define EGDS_MESSAGE_BOX_MIN_TEXT_WIDTH GUIDefaultSize::MessageBoxMinTextWidth
#define EGDS_MESSAGE_BOX_MAX_TEXT_WIDTH GUIDefaultSize::MessageBoxMaxTextWidth
#define EGDS_MESSAGE_BOX_MIN_TEXT_HEIGHT GUIDefaultSize::MessageBoxMinTextHeight
#define EGDS_MESSAGE_BOX_MAX_TEXT_HEIGHT GUIDefaultSize::MessageBoxMaxTextHeight
#define EGDS_BUTTON_PRESSED_IMAGE_OFFSET_X GUIDefaultSize::ButtonPressedImageOffsetX
#define EGDS_BUTTON_PRESSED_IMAGE_OFFSET_Y GUIDefaultSize::ButtonPressedImageOffsetY
#define EGDS_BUTTON_PRESSED_TEXT_OFFSET_X GUIDefaultSize::ButtonPressedTextOffsetX
#define EGDS_BUTTON_PRESSED_TEXT_OFFSET_Y GUIDefaultSize::ButtonPressedTextOffsetY
#define EGDS_BUTTON_PRESSED_SPRITE_OFFSET_X GUIDefaultSize::ButtonPressedSpriteOffsetX
#define EGDS_BUTTON_PRESSED_SPRITE_OFFSET_Y GUIDefaultSize::ButtonPressedSpriteOffsetY
#define EGDS_COUNT GUIDefaultSize::Count


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

#define EGUI_DEFAULT_TEXT GUIDefaultText

#define EGDT_MSG_BOX_OK GUIDefaultText::MsgBoxOk
#define EGDT_MSG_BOX_CANCEL GUIDefaultText::MsgBoxCancel
#define EGDT_MSG_BOX_YES GUIDefaultText::MsgBoxYes
#define EGDT_MSG_BOX_NO GUIDefaultText::MsgBoxNo
#define EGDT_WINDOW_CLOSE GUIDefaultText::WindowClose
#define EGDT_WINDOW_MAXIMIZE GUIDefaultText::WindowMaximize
#define EGDT_WINDOW_MINIMIZE GUIDefaultText::WindowMaximize
#define EGDT_WINDOW_RESTORE GUIDefaultText::WindowRestore
#define EGDT_COUNT GUIDefaultText::Count

//! Customizable symbols for GUI
enum class GUIDefaultIcon : u8
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

#define EGUI_DEFAULT_ICON GUIDefaultIcon

#define EGDI_WINDOW_MAXIMIZE GUIDefaultIcon::WindowMaximize
#define EGDI_WINDOW_RESTORE GUIDefaultIcon::WindowRestore
#define EGDI_WINDOW_CLOSE GUIDefaultIcon::WindowClose
#define EGDI_WINDOW_MINIMIZE GUIDefaultIcon::WindowMinimize
#define EGDI_WINDOW_RESIZE GUIDefaultIcon::WindowResize
#define EGDI_CURSOR_UP GUIDefaultIcon::CursorUp
#define EGDI_CURSOR_DOWN GUIDefaultIcon::CursorDown
#define EGDI_CURSOR_LEFT GUIDefaultIcon::CursorLeft
#define EGDI_CURSOR_RIGHT GUIDefaultIcon::CursorRight
#define EGDI_MENU_MORE GUIDefaultIcon::MenuMore
#define EGDI_CHECK_BOX_CHECKED GUIDefaultIcon::CheckBoxChecked
#define EGDI_DROP_DOWN GUIDefaultIcon::Dropdown
#define EGDI_SMALL_CURSOR_UP GUIDefaultIcon::SmallCursorUp
#define EGDI_SMALL_CURSOR_DOWN GUIDefaultIcon::SmallCursorDown
#define EGDI_RADIO_BUTTON_CHECKED GUIDefaultIcon::RadioButtonChecked
#define EGDI_MORE_LEFT GUIDefaultIcon::MoreLeft
#define EGDI_MORE_RIGHT GUIDefaultIcon::MoreRight
#define EGDI_MORE_UP GUIDefaultIcon::MoreUp
#define EGDI_MORE_DOWN GUIDefaultIcon::MoreDown
#define EGDI_EXPAND GUIDefaultIcon::Expand
#define EGDI_COLLAPSE GUIDefaultIcon::Collapse
#define EGDI_FILE GUIDefaultIcon::File
#define EGDI_DIRECTORY GUIDefaultIcon::Directory
#define EGDI_COUNT GUIDefaultIcon::Count

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

#define EGDF_DEFAULT GUIDefaultFont::Default
#define EGDF_BUTTON GUIDefaultFont::Button
#define EGDF_WINDOW GUIDefaultFont::Window
#define EGDF_MENU GUIDefaultFont::Menu
#define EGDF_TOOLTIP GUIDefaultFont::Tooltip
#define EGDF_COUNT GUIDefaultFont::Count

class GUISkin
{
public:
    GUISkin(Renderer *_renderer, GUISkinType type=GUISkinType::WindowsClassic);

    //! returns display density scaling factor
    f32 getScale() const { return Scale; }

    //! sets display density scaling factor
    void setScale(f32 scale) { Scale = scale; }

    //! returns default color
    img::color8 getColor(GUIDefaultColor color) const;

    //! sets a default color
    void setColor(GUIDefaultColor which, img::color8 newColor);

    //! returns size for the given size type
    s32 getSize(GUIDefaultSize size) const;

    //! sets a default size
    void setSize(GUIDefaultSize which, s32 size);

    //! returns the default font
    render::TTFont *getFont(GUIDefaultFont which=GUIDefaultFont::Default) const;

    //! sets a default font
    void setFont(render::TTFont *font, GUIDefaultFont which=GUIDefaultFont::Default);

    //! sets the sprite used for drawing icons
    void setSpriteBank(gui::IGUISpriteBank *bank);

    //! gets the sprite used for drawing icons
    gui::IGUISpriteBank *getSpriteBank() const;

    //! Returns a default icon
    /** Returns the sprite index within the sprite */
    u32 getIcon(GUIDefaultIcon icon) const;

    //! Sets a default icon
    /** Sets the sprite index used for drawing icons like arrows,
    close buttons and ticks in checkboxes
    \param icon: Enum specifying which icon to change
    \param index: The sprite index used to draw this icon */
    void setIcon(GUIDefaultIcon icon, u32 index);

    //! Returns a default text.
    /** For example for Message box button captions:
    "OK", "Cancel", "Yes", "No" and so on. */
    std::wstring getDefaultText(GUIDefaultText text) const;

    //! Sets a default text.
    /** For example for Message box button captions:
    "OK", "Cancel", "Yes", "No" and so on. */
    void setDefaultText(GUIDefaultText which, const std::wstring &newText);

    //! draws a standard 3d button pane
    /** Used for drawing for example buttons in normal state.
    It uses the colors EGDC_3D_DARK_SHADOW, EGDC_3D_HIGH_LIGHT, EGDC_3D_SHADOW and
    EGDC_3D_FACE for this. See EGUI_DEFAULT_COLOR for details.
    \param rect: Defining area where to draw.
    \param clip: Clip area.
    \param element: Pointer to the element which wishes to draw this. This parameter
    is usually not used by ISkin, but can be used for example by more complex
    implementations to find out how to draw the part exactly. */
    void add3DButtonPaneStandard(UISprite *sprite,
        const rectf& rect)
    {
        addColored3DButtonPaneStandard(sprite, rect);
    }

    void addColored3DButtonPaneStandard(UISprite *sprite,
        const rectf &rect,
        const img::color8 *colors=nullptr);

    //! draws a pressed 3d button pane
    /** Used for drawing for example buttons in pressed state.
    It uses the colors EGDC_3D_DARK_SHADOW, EGDC_3D_HIGH_LIGHT, EGDC_3D_SHADOW and
    EGDC_3D_FACE for this. See EGUI_DEFAULT_COLOR for details.
    \param rect: Defining area where to draw.
    \param clip: Clip area.
    \param element: Pointer to the element which wishes to draw this. This parameter
    is usually not used by ISkin, but can be used for example by more complex
    implementations to find out how to draw the part exactly. */
    void add3DButtonPanePressed(UISprite *sprite,
        const rectf& rect)
    {
        addColored3DButtonPanePressed(sprite, rect);
    }

    void addColored3DButtonPanePressed(UISprite *sprite,
        const rectf& rect,
        const img::color8 *colors=nullptr);

    //! draws a sunken 3d pane
    /** Used for drawing the background of edit, combo or check boxes.
    \param element: Pointer to the element which wishes to draw this. This parameter
    is usually not used by ISkin, but can be used for example by more complex
    implementations to find out how to draw the part exactly.
    \param bgcolor: Background color.
    \param flat: Specifies if the sunken pane should be flat or displayed as sunken
    deep into the ground.
    \param rect: Defining area where to draw.
    \param clip: Clip area.	*/
    void add3DSunkenPane(UISprite *sprite,
        img::color8 bgcolor, bool flat,
        bool fillBackGround,
        const rectf& rect)
    {
        addColored3DSunkenPane(sprite, bgcolor, flat, fillBackGround, rect);
    }

    void addColored3DSunkenPane(UISprite *sprite,
        img::color8 bgcolor, bool flat,
        bool fillBackGround,
        const rectf& rect,
        const img::color8 *colors=nullptr);

    //! draws a window background
    /** Used for drawing the background of dialogs and windows.
    \param element: Pointer to the element which wishes to draw this. This parameter
    is usually not used by ISkin, but can be used for example by more complex
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
    rectf add3DWindowBackground(UISprite *sprite,
        bool drawTitleBar, img::color8 titleBarColor,
        const rectf& rect,
        rectf* checkClientArea)
    {
        return addColored3DWindowBackground(sprite, drawTitleBar, titleBarColor,
            rect, checkClientArea);
    }

    rectf addColored3DWindowBackground(UISprite *sprite,
        bool drawTitleBar, img::color8 titleBarColor,
        const rectf& rect,
        rectf* checkClientArea,
        const img::color8 *colors=nullptr);

    //! draws a standard 3d menu pane
    /** Used for drawing for menus and context menus.
    It uses the colors EGDC_3D_DARK_SHADOW, EGDC_3D_HIGH_LIGHT, EGDC_3D_SHADOW and
    EGDC_3D_FACE for this. See EGUI_DEFAULT_COLOR for details.
    \param element: Pointer to the element which wishes to draw this. This parameter
    is usually not used by ISkin, but can be used for example by more complex
    implementations to find out how to draw the part exactly.
    \param rect: Defining area where to draw.
    \param clip: Clip area.	*/
    void add3DMenuPane(UISprite *sprite,
        const rectf& rect)
    {
        addColored3DMenuPane(sprite, rect);
    }

    void addColored3DMenuPane(UISprite *sprite,
        const rectf& rect,
        const img::color8 *colors=nullptr);

    //! draws a standard 3d tool bar
    /** Used for drawing for toolbars and menus.
    \param element: Pointer to the element which wishes to draw this. This parameter
    is usually not used by ISkin, but can be used for example by more complex
    implementations to find out how to draw the part exactly.
    \param rect: Defining area where to draw.
    \param clip: Clip area.	*/
    void add3DToolBar(UISprite *sprite,
        const rectf& rect)
    {
        addColored3DToolBar(sprite, rect);
    }

    void addColored3DToolBar(UISprite *sprite,
        const rectf& rect,
        const img::color8 *colors=nullptr);

    //! draws a tab button
    /** Used for drawing for tab buttons on top of tabs.
    \param element: Pointer to the element which wishes to draw this. This parameter
    is usually not used by ISkin, but can be used for example by more complex
    implementations to find out how to draw the part exactly.
    \param active: Specifies if the tab is currently active.
    \param rect: Defining area where to draw.
    \param clip: Clip area.	*/
    void add3DTabButton(UISprite *sprite, bool active,
        const rectf& rect, GUIAlignment alignment=GUIAlignment::UpperLeft)
    {
        addColored3DTabButton(sprite, active, rect, alignment);
    }

    void addColored3DTabButton(UISprite *sprite, bool active,
        const rectf& rect, GUIAlignment alignment=GUIAlignment::UpperLeft,
        const img::color8 *colors=nullptr);

    //! draws a tab control body
    /** \param element: Pointer to the element which wishes to draw this. This parameter
    is usually not used by ISkin, but can be used for example by more complex
    implementations to find out how to draw the part exactly.
    \param border: Specifies if the border should be drawn.
    \param background: Specifies if the background should be drawn.
    \param rect: Defining area where to draw.
    \param clip: Clip area.	*/
    void add3DTabBody(UISprite *sprite, bool border, bool background,
        const rectf& rect, s32 tabHeight=-1, GUIAlignment alignment=GUIAlignment::UpperLeft)
    {
        addColored3DTabBody(sprite, border, background, rect, tabHeight, alignment);
    }

    void addColored3DTabBody(UISprite *sprite, bool border, bool background,
        const rectf& rect, s32 tabHeight=-1, GUIAlignment alignment=GUIAlignment::UpperLeft,
        const img::color8 *colors=nullptr);

    //! draws an icon, usually from the skin's sprite bank
    /** \param element: Pointer to the element which wishes to draw this icon.
    This parameter is usually not used by IGUISkin, but can be used for example
    by more complex implementations to find out how to draw the part exactly.
    \param icon: Specifies the icon to be drawn.
    \param position: The position to draw the icon
    \param starttime: The time at the start of the animation
    \param currenttime: The present time, used to calculate the frame number
    \param loop: Whether the animation should loop or not
    \param clip: Clip area.	*/
    virtual void drawIcon(gui::IGUIElement* element, EGUI_DEFAULT_ICON icon,
        const v2i &position,
        u32 starttime=0, u32 currenttime=0,
        bool loop=false, const recti* clip=0)
    {
        drawColoredIcon(element, icon, position, starttime, currenttime, loop, clip);
    }

    virtual void drawColoredIcon(gui::IGUIElement* element, EGUI_DEFAULT_ICON icon,
        const v2i &position,
        u32 starttime=0, u32 currenttime=0,
        bool loop=false, const recti* clip=0,
        const img::color8* colors=0);

    //! get the type of this skin
    GUISkinType getType() const;

    //! gets the colors
    void getColors(img::color8* colors); // ::PATCH:

private:
    Renderer *renderer;

    f32 Scale = 1.0f;
    std::array<img::color8, (u8)GUIDefaultColor::Count>     Colors;
    std::array<s32, (u8)GUIDefaultSize::Count>              Sizes;
    std::array<u32, (u8)GUIDefaultIcon::Count>              Icons;
    std::array<render::TTFont *, (u8)GUIDefaultFont::Count> Fonts;
    gui::IGUISpriteBank                                     *Sprite = nullptr;
    std::array<std::wstring, (u8)GUIDefaultText::Count>     Texts;

    bool UseGradient;

    GUISkinType Type;

    inline void addRect(UISprite *sprite, const std::array<img::color8, 4> &newColors, const rectf &newRect)
    {
        sprite->getShape().addRectangle(newRect, newColors);
    }
    inline void addRect(UISprite *sprite, const img::color8 &newColor, const rectf &newRect)
    {
        addRect(sprite, {newColor, newColor, newColor, newColor}, newRect);
    }
};
