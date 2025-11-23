// Copyright (C) 2002-2012 Nikolaus Gebhardt, Modified by Mustapha Tachouct
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef GUIEDITBOXWITHSCROLLBAR_HEADER
#define GUIEDITBOXWITHSCROLLBAR_HEADER

#include "guiEditBox.h"

class UISpriteBank;

class GUIEditBoxWithScrollBar : public GUIEditBox
{
public:

	//! constructor
	GUIEditBoxWithScrollBar(const wchar_t* text, bool border, IGUIEnvironment* environment,
		IGUIElement* parent, s32 id, const recti& rectangle,
        bool writable = true, bool has_vscrollbar = true);

	//! destructor
	virtual ~GUIEditBoxWithScrollBar() {}

	//! Sets whether to draw the background
    virtual void setDrawBackground(bool draw) override;

    void updateMesh() override;

	//! draws the element and its children
    virtual void draw() override;

	//! Updates the absolute position, splits text if required
    virtual void updateAbsolutePosition() override;

	//! Change the background color
	virtual void setBackgroundColor(const img::color8 &bg_color);

    virtual bool isDrawBackgroundEnabled() const override;
    virtual bool isDrawBorderEnabled() const override;
    virtual void setCursorChar(const wchar_t cursorChar) override;
    virtual wchar_t getCursorChar() const override;
    virtual void setCursorBlinkTime(u32 timeMs) override;
    virtual u32 getCursorBlinkTime() const override;

protected:
	//! Breaks the single text line.
    virtual void breakText() override;
	//! sets the area of the given line
    virtual void setTextRect(s32 line) override;
	//! calculates the current scroll position
    void calculateScrollPos() override;
	//! calculated the FrameRect
	void calculateFrameRect();
	//! create a Vertical ScrollBar
	void createVScrollBar();

    s32 getCursorPos(s32 x, s32 y) override;

	bool m_background;

	bool m_bg_color_used;
	img::color8 m_bg_color;

    std::unique_ptr<UISpriteBank> m_editbox_bank;
};


#endif // GUIEDITBOXWITHSCROLLBAR_HEADER

