// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "guiTabControl.h"

#include "client/render/rendersystem.h"
#include "client/ui/extra_images.h"
#include "client/ui/text_sprite.h"
#include "guiButton.h"
#include "GUISkin.h"
#include "IGUIEnvironment.h"
#include "util/enriched_string.h"
#include <Utils/Rect.h>

namespace gui
{

// ------------------------------------------------------------------
// Tab
// ------------------------------------------------------------------

//! constructor
CGUITab::CGUITab(IGUIEnvironment *environment,
		IGUIElement *parent, const recti &rectangle,
		s32 id) :
		IGUITab(environment, parent, id, rectangle),
        BackColor(img::black), OverrideTextColorEnabled(false), TextColor(img::black),
        DrawBackground(false),
        drawBatch(std::make_unique<SpriteDrawBatch>(environment->getRenderSystem(), environment->getResourceCache()))
{
	const GUISkin *const skin = environment->getSkin();
	if (skin)
		TextColor = skin->getColor(EGDC_BUTTON_TEXT);

    box = drawBatch->addRectsSprite({{}});
}

void CGUITab::updateMesh()
{
    if (!Rebuild)
        return;
    GUISkin *skin = Environment->getSkin();

    if (skin && DrawBackground) {
        box->updateRect(0, {toRectT<f32>(AbsoluteRect), BackColor});
        box->setClipRect(AbsoluteClippingRect);
        drawBatch->rebuild();
    }

    Rebuild = false;
}

//! draws the element and its children
void CGUITab::draw()
{
	if (!IsVisible)
		return;

    updateMesh();

    if (Environment->getSkin() && DrawBackground) {
        drawBatch->draw();
    }

	IGUIElement::draw();
}

//! sets if the tab should draw its background
void CGUITab::setDrawBackground(bool draw)
{
    if (draw != DrawBackground) Rebuild = true;
	DrawBackground = draw;
}

//! sets the color of the background, if it should be drawn.
void CGUITab::setBackgroundColor(img::color8 c)
{
    if (c != BackColor) Rebuild = true;
	BackColor = c;
}

//! sets the color of the text
void CGUITab::setTextColor(img::color8 c)
{
    if (c != TextColor) Rebuild = true;
	OverrideTextColorEnabled = true;
	TextColor = c;
}

img::color8 CGUITab::getTextColor() const
{
	if (OverrideTextColorEnabled)
		return TextColor;
	else
		return Environment->getSkin()->getColor(EGDC_BUTTON_TEXT);
}

//! returns true if the tab is drawing its background, false if not
bool CGUITab::isDrawingBackground() const
{
	return DrawBackground;
}

//! returns the color of the background
img::color8 CGUITab::getBackgroundColor() const
{
	return BackColor;
}

// ------------------------------------------------------------------
// Tabcontrol
// ------------------------------------------------------------------

//! constructor
CGUITabControl::CGUITabControl(IGUIEnvironment *environment,
		IGUIElement *parent, const recti &rectangle,
		bool fillbackground, bool border, s32 id) :
		IGUITabControl(environment, parent, id, rectangle),
		ActiveTabIndex(-1),
        Border(border), FillBackground(fillbackground), ScrollControl(false), TabHeight(0), VerticalAlignment(GUIAlignment::UpperLeft),
        UpButton(0), DownButton(0), TabMaxWidth(0), CurrentScrollTabIndex(0), TabExtraWidth(20),
        drawBatch(std::make_unique<SpriteDrawBatch>(environment->getRenderSystem(), environment->getResourceCache()))
{
	GUISkin *skin = Environment->getSkin();
	IGUISpriteBank *sprites = 0;

	TabHeight = 32;

	if (skin) {
		sprites = skin->getSpriteBank();
        TabHeight = skin->getSize(EGDS_BUTTON_HEIGHT) + 2;
	}

	UpButton = Environment->addButton(recti(0, 0, 10, 10), this);

	if (UpButton) {
		UpButton->setSpriteBank(sprites);
		UpButton->setVisible(false);
		UpButton->setSubElement(true);
        UpButton->setAlignment(EGUIA_LOWERRIGHT, EGUIA_LOWERRIGHT, GUIAlignment::UpperLeft, GUIAlignment::UpperLeft);
        UpButton->setOverrideFont(skin->getFont());
		UpButton->grab();
	}

	DownButton = Environment->addButton(recti(0, 0, 10, 10), this);

	if (DownButton) {
		DownButton->setSpriteBank(sprites);
		DownButton->setVisible(false);
		DownButton->setSubElement(true);
        DownButton->setAlignment(EGUIA_LOWERRIGHT, EGUIA_LOWERRIGHT, GUIAlignment::UpperLeft, GUIAlignment::UpperLeft);
        DownButton->setOverrideFont(skin->getFont());
		DownButton->grab();
	}

    setTabVerticalAlignment(GUIAlignment::UpperLeft);
	refreshSprites();
}

//! destructor
CGUITabControl::~CGUITabControl()
{
	for (u32 i = 0; i < Tabs.size(); ++i) {
		if (Tabs[i])
			Tabs[i]->drop();
	}

	if (UpButton)
		UpButton->drop();

	if (DownButton)
		DownButton->drop();
}

void CGUITabControl::refreshSprites()
{
    img::color8 color(img::white);
	GUISkin *skin = Environment->getSkin();
	if (skin) {
		color = skin->getColor(isEnabled() ? EGDC_WINDOW_SYMBOL : EGDC_GRAY_WINDOW_SYMBOL);

		if (UpButton) {
			UpButton->setSprite(EGBS_BUTTON_UP, skin->getIcon(EGDI_CURSOR_LEFT), color);
			UpButton->setSprite(EGBS_BUTTON_DOWN, skin->getIcon(EGDI_CURSOR_LEFT), color);
		}

		if (DownButton) {
			DownButton->setSprite(EGBS_BUTTON_UP, skin->getIcon(EGDI_CURSOR_RIGHT), color);
			DownButton->setSprite(EGBS_BUTTON_DOWN, skin->getIcon(EGDI_CURSOR_RIGHT), color);
		}
	}
}

//! Adds a tab
IGUITab *CGUITabControl::addTab(const wchar_t *caption, s32 id)
{
	CGUITab *tab = new CGUITab(Environment, this, calcTabPos(), id);

	tab->setText(caption);
    tab->setAlignment(GUIAlignment::UpperLeft, EGUIA_LOWERRIGHT, GUIAlignment::UpperLeft, EGUIA_LOWERRIGHT);
	tab->setVisible(false);
	Tabs.push_back(tab); // no grab as new already creates a reference

	if (ActiveTabIndex == -1) {
		ActiveTabIndex = Tabs.size() - 1;
        Rebuild = true;
		tab->setVisible(true);
	}

	recalculateScrollBar();

	return tab;
}

//! adds a tab which has been created elsewhere
s32 CGUITabControl::addTab(IGUITab *tab)
{
	return insertTab(Tabs.size(), tab, false);
}

//! Insert the tab at the given index
IGUITab *CGUITabControl::insertTab(s32 idx, const wchar_t *caption, s32 id)
{
	if (idx < 0 || idx > (s32)Tabs.size()) // idx == Tabs.size() is indeed OK here as std::vector can handle that
		return NULL;

	CGUITab *tab = new CGUITab(Environment, this, calcTabPos(), id);

	tab->setText(caption);
    tab->setAlignment(GUIAlignment::UpperLeft, EGUIA_LOWERRIGHT, GUIAlignment::UpperLeft, EGUIA_LOWERRIGHT);
	tab->setVisible(false);
    Tabs.insert(Tabs.begin()+(u32)idx, tab);

	if (ActiveTabIndex == -1) {
		ActiveTabIndex = (u32)idx;
		tab->setVisible(true);
        Rebuild = true;
	} else if (idx <= ActiveTabIndex) {
		++ActiveTabIndex;
		setVisibleTab(ActiveTabIndex);
        Rebuild = true;
	}

	recalculateScrollBar();

	return tab;
}

s32 CGUITabControl::insertTab(s32 idx, IGUITab *tab, bool serializationMode)
{
	if (!tab)
		return -1;
	if (idx > (s32)Tabs.size() && !serializationMode) // idx == Tabs.size() is indeed OK here as std::vector can handle that
		return -1;
	// Not allowing to add same tab twice as it would make things complicated (serialization or setting active visible)
	if (getTabIndex(tab) >= 0)
		return -1;

	if (idx < 0)
		idx = (s32)Tabs.size();

	if (tab->getParent() != this)
		this->addChildToEnd(tab);

	tab->setVisible(false);

	tab->grab();

	if (serializationMode) {
		while (idx >= (s32)Tabs.size()) {
			Tabs.push_back(0);
		}
		Tabs[idx] = tab;

		if (idx == ActiveTabIndex) { // in serialization that can happen for any index
			setVisibleTab(ActiveTabIndex);
			tab->setVisible(true);
		}
	} else {
        Tabs.insert(Tabs.begin()+(u32)idx, tab);

		if (ActiveTabIndex == -1) {
			ActiveTabIndex = idx;
			setVisibleTab(ActiveTabIndex);
            Rebuild = true;
		} else if (idx <= ActiveTabIndex) {
			++ActiveTabIndex;
			setVisibleTab(ActiveTabIndex);
            Rebuild = true;
		}
	}

	recalculateScrollBar();

	return idx;
}

//! Removes a child.
void CGUITabControl::removeChild(IGUIElement *child)
{
	s32 idx = getTabIndex(child);
	if (idx >= 0)
		removeTabButNotChild(idx);

	// remove real element
	IGUIElement::removeChild(child);

	recalculateScrollBar();
}

//! Removes a tab from the tabcontrol
void CGUITabControl::removeTab(s32 idx)
{
	if (idx < 0 || idx >= (s32)Tabs.size())
		return;

	removeChild(Tabs[(u32)idx]);
}

void CGUITabControl::removeTabButNotChild(s32 idx)
{
	if (idx < 0 || idx >= (s32)Tabs.size())
		return;

	Tabs[(u32)idx]->drop();
    Tabs.erase(Tabs.begin()+(u32)idx);

	if (idx < ActiveTabIndex) {
		--ActiveTabIndex;
		setVisibleTab(ActiveTabIndex);
        Rebuild = true;
	} else if (idx == ActiveTabIndex) {
		if ((u32)idx == Tabs.size())
			--ActiveTabIndex;
		setVisibleTab(ActiveTabIndex);
        Rebuild = true;
	}
}

//! Clears the tabcontrol removing all tabs
void CGUITabControl::clear()
{
	for (u32 i = 0; i < Tabs.size(); ++i) {
		if (Tabs[i]) {
			IGUIElement::removeChild(Tabs[i]);
			Tabs[i]->drop();
		}
	}
	Tabs.clear();

	recalculateScrollBar();
}

//! Returns amount of tabs in the tabcontrol
s32 CGUITabControl::getTabCount() const
{
	return Tabs.size();
}

//! Returns a tab based on zero based index
IGUITab *CGUITabControl::getTab(s32 idx) const
{
	if (idx < 0 || (u32)idx >= Tabs.size())
		return 0;

	return Tabs[idx];
}

//! called if an event happened.
bool CGUITabControl::OnEvent(const core::Event &event)
{
	if (isEnabled()) {
		switch (event.Type) {
		case EET_GUI_EVENT:
			switch (event.GUI.Type) {
			case EGET_BUTTON_CLICKED:
                if (event.GUI.Caller == UpButton) {
					scrollLeft();
					return true;
                } else if (event.GUI.Caller == DownButton) {
					scrollRight();
					return true;
				}

				break;
			default:
				break;
			}
			break;
		case EET_MOUSE_INPUT_EVENT:
			switch (event.MouseInput.Type) {
			// case EMIE_LMOUSE_PRESSED_DOWN:
			//	// todo: dragging tabs around
			//	return true;
			case EMIE_LMOUSE_LEFT_UP: {
				s32 idx = getTabAt(event.MouseInput.X, event.MouseInput.Y);
				if (idx >= 0) {
					setActiveTab(idx);
					return true;
				}
				break;
			}
			default:
				break;
			}
			break;
		default:
			break;
		}
	}

	return IGUIElement::OnEvent(event);
}

void CGUITabControl::scrollLeft()
{
    if (CurrentScrollTabIndex > 0) {
		--CurrentScrollTabIndex;
        Rebuild = true;
    }
	recalculateScrollBar();
}

void CGUITabControl::scrollRight()
{
	if (CurrentScrollTabIndex < (s32)(Tabs.size()) - 1) {
        if (needScrollControl(CurrentScrollTabIndex, true)) {
			++CurrentScrollTabIndex;
            Rebuild = true;
        }
	}
	recalculateScrollBar();
}

s32 CGUITabControl::calcTabWidth(render::TTFont *font, const wchar_t *text) const
{
	if (!font)
		return 0;

    s32 len = font->getTextWidth(text) + TabExtraWidth;
	if (TabMaxWidth > 0 && len > TabMaxWidth)
		len = TabMaxWidth;

	return len;
}

bool CGUITabControl::needScrollControl(s32 startIndex, bool withScrollControl, s32 *pos_rightmost)
{
	if (startIndex < 0)
		startIndex = 0;

	GUISkin *skin = Environment->getSkin();
	if (!skin)
		return false;

	render::TTFont *font = skin->getFont();

	if (Tabs.empty())
		return false;

	if (!font)
		return false;

	s32 pos = AbsoluteRect.ULC.X + 2;
	const s32 pos_right = withScrollControl ? UpButton->getAbsolutePosition().ULC.X - 2 : AbsoluteRect.LRC.X;

	for (s32 i = startIndex; i < (s32)Tabs.size(); ++i) {
		// get Text
		const wchar_t *text = 0;
		if (Tabs[i]) {
			text = Tabs[i]->getText();

			// get text length
			s32 len = calcTabWidth(font, text); // always without withScrollControl here or len would be shortened
			pos += len;
		}

		if (pos > pos_right)
			return true;
	}

	if (pos_rightmost)
		*pos_rightmost = pos;
	return false;
}

s32 CGUITabControl::calculateScrollIndexFromActive()
{
	if (!ScrollControl || Tabs.empty())
		return 0;

	GUISkin *skin = Environment->getSkin();
	if (!skin)
		return false;

	render::TTFont *font = skin->getFont();
	if (!font)
		return false;

	const s32 pos_left = AbsoluteRect.ULC.X + 2;
	const s32 pos_right = UpButton->getAbsolutePosition().ULC.X - 2;

	// Move from center to the left border left until it is reached
	s32 pos_cl = (pos_left + pos_right) / 2;
	s32 i = ActiveTabIndex;
	for (; i > 0; --i) {
		if (!Tabs[i])
			continue;

		s32 len = calcTabWidth(font, Tabs[i]->getText());
		if (i == ActiveTabIndex)
			len /= 2;
		if (pos_cl - len < pos_left)
			break;

		pos_cl -= len;
	}
	if (i == 0)
		return i;

	// Is scrolling to right still possible?
	s32 pos_rr = 0;
	if (needScrollControl(i, true, &pos_rr))
		return i; // Yes? -> OK

	// No? -> Decrease "i" more. Append tabs until scrolling becomes necessary
	for (--i; i > 0; --i) {
		if (!Tabs[i])
			continue;

		pos_rr += calcTabWidth(font, Tabs[i]->getText());
		if (pos_rr > pos_right)
			break;
	}
	return i + 1;
}

recti CGUITabControl::calcTabPos()
{
	recti r;
	r.ULC.X = 0;
	r.LRC.X = AbsoluteRect.getWidth();
	if (Border) {
		++r.ULC.X;
		--r.LRC.X;
	}

    if (VerticalAlignment == GUIAlignment::UpperLeft) {
		r.ULC.Y = TabHeight + 2;
		r.LRC.Y = AbsoluteRect.getHeight() - 1;
		if (Border) {
			--r.LRC.Y;
		}
	} else {
		r.ULC.Y = 0;
		r.LRC.Y = AbsoluteRect.getHeight() - (TabHeight + 2);
		if (Border) {
			++r.ULC.Y;
		}
	}

	return r;
}

//! draws the element and its children
void CGUITabControl::updateMesh()
{
    if (!Rebuild) return;

	GUISkin *skin = Environment->getSkin();
	if (!skin)
		return;

	render::TTFont *font = skin->getFont();

	recti frameRect(AbsoluteRect);

    drawBatch->clear();

	// some empty background as placeholder when there are no tabs
    if (Tabs.empty()) {
        img::color8 color = skin->getColor(EGDC_3D_HIGH_LIGHT);
        drawBatch->addRectsSprite({{toRectT<f32>(frameRect), color}});
    }

	if (!font)
		return;

	// tab button bar can be above or below the tabs
    if (VerticalAlignment == GUIAlignment::UpperLeft) {
		frameRect.ULC.Y += 2;
		frameRect.LRC.Y = frameRect.ULC.Y + TabHeight;
	} else {
		frameRect.ULC.Y = frameRect.LRC.Y - TabHeight - 1;
		frameRect.LRC.Y -= 2;
	}

	recti tr;
	s32 pos = frameRect.ULC.X + 2;

	bool needLeftScroll = CurrentScrollTabIndex > 0;
	bool needRightScroll = false;

	// left and right pos of the active tab
	s32 left = 0;
	s32 right = 0;

	// const wchar_t* activetext = 0;
	IGUITab *activeTab = 0;

	// Draw all tab-buttons except the active one
	for (u32 i = CurrentScrollTabIndex; i < Tabs.size() && !needRightScroll; ++i) {
		// get Text
		const wchar_t *text = 0;
		if (Tabs[i])
			text = Tabs[i]->getText();

		// get text length
		s32 len = calcTabWidth(font, text);
		if (ScrollControl) {
			s32 space = UpButton->getAbsolutePosition().ULC.X - 2 - pos;
			if (space < len) {
				needRightScroll = true;
				len = space;
			}
		}

		frameRect.LRC.X += len;
		frameRect.ULC.X = pos;
		frameRect.LRC.X = frameRect.ULC.X + len;

		pos += len;

		if ((s32)i == ActiveTabIndex) {
			// for active button just remember values
			left = frameRect.ULC.X;
			right = frameRect.LRC.X;
			// activetext = text;
			activeTab = Tabs[i];
		} else {
            auto tabButton = drawBatch->addRectsSprite({});
            tabButton->setClipRect(AbsoluteClippingRect);
            skin->add3DTabButton(tabButton, false, toRectT<f32>(frameRect), VerticalAlignment);

			// draw text
			recti textClipRect(frameRect); // TODO: exact size depends on borders in draw3DTabButton which we don't get with current interface
			textClipRect.clipAgainst(AbsoluteClippingRect);

            drawBatch->addTextSprite(text, 0, toRectT<f32>(frameRect),Tabs[i]->getTextColor(), &textClipRect,
                false, GUIAlignment::Center);
		}
	}

	// Draw active tab button
	// Drawn later than other buttons because it draw over the buttons before/after it.
	if (left != 0 && right != 0 && activeTab != 0) {
		// draw upper highlight frame
        if (VerticalAlignment == GUIAlignment::UpperLeft) {
			frameRect.ULC.X = left - 2;
			frameRect.LRC.X = right + 2;
			frameRect.ULC.Y -= 2;

            auto tabButton = drawBatch->addRectsSprite({});
            tabButton->setClipRect(AbsoluteClippingRect);
            skin->add3DTabButton(tabButton, true, toRectT<f32>(frameRect), VerticalAlignment);

			// draw text
			recti textClipRect(frameRect); // TODO: exact size depends on borders in draw3DTabButton which we don't get with current interface
			textClipRect.clipAgainst(AbsoluteClippingRect);
            auto TabText = drawBatch->addTextSprite(activeTab->getText(), 0,
                toRectT<f32>(frameRect), activeTab->getTextColor(), &textClipRect,
                false, GUIAlignment::Center);
            TabText->getTextObj().setAlignment(GUIAlignment::Center, GUIAlignment::Center);

			tr.ULC.X = AbsoluteRect.ULC.X;
			tr.LRC.X = left - 1;
			tr.ULC.Y = frameRect.LRC.Y - 1;
			tr.LRC.Y = frameRect.LRC.Y;

            tabButton = drawBatch->addRectsSprite({});
            tabButton->setClipRect(AbsoluteClippingRect);

            img::color8 color = skin->getColor(EGDC_3D_HIGH_LIGHT);
            tabButton->addRect({toRectT<f32>(tr), color});

			tr.ULC.X = right;
			tr.LRC.X = AbsoluteRect.LRC.X;
            tabButton->addRect({toRectT<f32>(tr), color});
		} else {
			frameRect.ULC.X = left - 2;
			frameRect.LRC.X = right + 2;
			frameRect.LRC.Y += 2;

            auto tabButton = drawBatch->addRectsSprite({});
            tabButton->setClipRect(AbsoluteClippingRect);
            skin->add3DTabButton(tabButton, true, toRectT<f32>(frameRect), VerticalAlignment);

			// draw text
            drawBatch->addTextSprite(activeTab->getText(), 0,
                toRectT<f32>(frameRect), activeTab->getTextColor(), &frameRect,
                false, GUIAlignment::Center);

            tabButton = drawBatch->addRectsSprite({});
            tabButton->setClipRect(AbsoluteClippingRect);

			tr.ULC.X = AbsoluteRect.ULC.X;
			tr.LRC.X = left - 1;
			tr.ULC.Y = frameRect.ULC.Y - 1;
			tr.LRC.Y = frameRect.ULC.Y;

            img::color8 color = skin->getColor(EGDC_3D_DARK_SHADOW);
            tabButton->addRect({toRectT<f32>(tr), color});

			tr.ULC.X = right;
			tr.LRC.X = AbsoluteRect.LRC.X;
            tabButton->addRect({toRectT<f32>(tr), color});
		}
	} else {
		// No active tab
		// Draw a line separating button bar from tab area
		tr.ULC.X = AbsoluteRect.ULC.X;
		tr.LRC.X = AbsoluteRect.LRC.X;
		tr.ULC.Y = frameRect.LRC.Y - 1;
		tr.LRC.Y = frameRect.LRC.Y;

        auto tabButton = drawBatch->addRectsSprite({});
        tabButton->setClipRect(AbsoluteClippingRect);

        if (VerticalAlignment == GUIAlignment::UpperLeft) {
            img::color8 color = skin->getColor(EGDC_3D_HIGH_LIGHT);
            tabButton->addRect({toRectT<f32>(tr), color});
		} else {
			tr.ULC.Y = frameRect.ULC.Y - 1;
			tr.LRC.Y = frameRect.ULC.Y;
            img::color8 color = skin->getColor(EGDC_3D_DARK_SHADOW);
            tabButton->addRect({toRectT<f32>(tr), color});
		}
	}

    auto tabBody = drawBatch->addRectsSprite({});
	// drawing some border and background for the tab-area.
    skin->add3DTabBody(tabBody, Border, FillBackground, toRectT<f32>(AbsoluteRect), TabHeight, VerticalAlignment);
    tabBody->setClipRect(AbsoluteClippingRect);

    drawBatch->rebuild();

	// enable scrollcontrols on need
	if (UpButton)
		UpButton->setEnabled(needLeftScroll);
	if (DownButton)
		DownButton->setEnabled(needRightScroll);
	refreshSprites();

    Rebuild = false;
}

//! draws the element and its children
void CGUITabControl::draw()
{
    if (!IsVisible)
        return;

    updateMesh();
    drawBatch->draw();

    IGUIElement::draw();
}

//! Set the height of the tabs
void CGUITabControl::setTabHeight(s32 height)
{
	if (height < 0)
		height = 0;

	TabHeight = height;

	recalculateScrollButtonPlacement();
	recalculateScrollBar();
}

//! Get the height of the tabs
s32 CGUITabControl::getTabHeight() const
{
	return TabHeight;
}

//! set the maximal width of a tab. Per default width is 0 which means "no width restriction".
void CGUITabControl::setTabMaxWidth(s32 width)
{
	TabMaxWidth = width;
}

//! get the maximal width of a tab
s32 CGUITabControl::getTabMaxWidth() const
{
	return TabMaxWidth;
}

//! Set the extra width added to tabs on each side of the text
void CGUITabControl::setTabExtraWidth(s32 extraWidth)
{
	if (extraWidth < 0)
		extraWidth = 0;

	TabExtraWidth = extraWidth;

	recalculateScrollBar();
}

//! Get the extra width added to tabs on each side of the text
s32 CGUITabControl::getTabExtraWidth() const
{
	return TabExtraWidth;
}

void CGUITabControl::recalculateScrollBar()
{
	// Down: to right, Up: to left
	if (!UpButton || !DownButton)
		return;

	ScrollControl = needScrollControl() || CurrentScrollTabIndex > 0;

	if (ScrollControl) {
		UpButton->setVisible(true);
		DownButton->setVisible(true);
	} else {
		UpButton->setVisible(false);
		DownButton->setVisible(false);
	}

	bringToFront(UpButton);
	bringToFront(DownButton);
}

//! Set the alignment of the tabs
void CGUITabControl::setTabVerticalAlignment(EGUI_ALIGNMENT alignment)
{
	VerticalAlignment = alignment;

	recalculateScrollButtonPlacement();
	recalculateScrollBar();

	recti r(calcTabPos());
	for (u32 i = 0; i < Tabs.size(); ++i) {
		Tabs[i]->setRelativePosition(r);
	}
}

void CGUITabControl::recalculateScrollButtonPlacement()
{
	GUISkin *skin = Environment->getSkin();
	s32 ButtonSize = 16;
	s32 ButtonHeight = TabHeight - 2;
	if (ButtonHeight < 0)
		ButtonHeight = TabHeight;
	if (skin) {
		ButtonSize = skin->getSize(EGDS_WINDOW_BUTTON_WIDTH);
		if (ButtonSize > TabHeight)
			ButtonSize = TabHeight;
	}

	s32 ButtonX = RelativeRect.getWidth() - (s32)(2.5f * (f32)ButtonSize) - 1;
	s32 ButtonY = 0;

    if (VerticalAlignment == GUIAlignment::UpperLeft) {
		ButtonY = 2 + (TabHeight / 2) - (ButtonHeight / 2);
        UpButton->setAlignment(EGUIA_LOWERRIGHT, EGUIA_LOWERRIGHT, GUIAlignment::UpperLeft, GUIAlignment::UpperLeft);
        DownButton->setAlignment(EGUIA_LOWERRIGHT, EGUIA_LOWERRIGHT, GUIAlignment::UpperLeft, GUIAlignment::UpperLeft);
	} else {
		ButtonY = RelativeRect.getHeight() - (TabHeight / 2) - (ButtonHeight / 2) - 2;
		UpButton->setAlignment(EGUIA_LOWERRIGHT, EGUIA_LOWERRIGHT, EGUIA_LOWERRIGHT, EGUIA_LOWERRIGHT);
		DownButton->setAlignment(EGUIA_LOWERRIGHT, EGUIA_LOWERRIGHT, EGUIA_LOWERRIGHT, EGUIA_LOWERRIGHT);
	}

	UpButton->setRelativePosition(recti(ButtonX, ButtonY, ButtonX + ButtonSize, ButtonY + ButtonHeight));
	ButtonX += ButtonSize + 1;
	DownButton->setRelativePosition(recti(ButtonX, ButtonY, ButtonX + ButtonSize, ButtonY + ButtonHeight));
}

//! Get the alignment of the tabs
EGUI_ALIGNMENT CGUITabControl::getTabVerticalAlignment() const
{
	return VerticalAlignment;
}

s32 CGUITabControl::getTabAt(s32 xpos, s32 ypos) const
{
	v2i p(xpos, ypos);
	GUISkin *skin = Environment->getSkin();
	render::TTFont *font = skin->getFont();

	recti frameRect(AbsoluteRect);

    if (VerticalAlignment == GUIAlignment::UpperLeft) {
		frameRect.ULC.Y += 2;
		frameRect.LRC.Y = frameRect.ULC.Y + TabHeight;
	} else {
		frameRect.ULC.Y = frameRect.LRC.Y - TabHeight;
	}

	s32 pos = frameRect.ULC.X + 2;

	if (!frameRect.isPointInside(p))
		return -1;

	bool abort = false;
	for (s32 i = CurrentScrollTabIndex; i < (s32)Tabs.size() && !abort; ++i) {
		// get Text
		const wchar_t *text = 0;
		if (Tabs[i])
			text = Tabs[i]->getText();

		// get text length
		s32 len = calcTabWidth(font, text);
		if (ScrollControl) {
			// TODO: merge this with draw() ?
			s32 space = UpButton->getAbsolutePosition().ULC.X - 2 - pos;
			if (space < len) {
				abort = true;
				len = space;
			}
		}

		frameRect.ULC.X = pos;
		frameRect.LRC.X = frameRect.ULC.X + len;

		pos += len;

		if (frameRect.isPointInside(p)) {
			return i;
		}
	}
	return -1;
}

//! Returns which tab is currently active
s32 CGUITabControl::getActiveTab() const
{
	return ActiveTabIndex;
}

//! Brings a tab to front.
bool CGUITabControl::setActiveTab(s32 idx)
{
	if ((u32)idx >= Tabs.size())
		return false;

	bool changed = (ActiveTabIndex != idx);

	ActiveTabIndex = idx;
    Rebuild = true;

	setVisibleTab(ActiveTabIndex);

	if (changed && Parent) {
		core::Event event;
		event.Type = EET_GUI_EVENT;
        event.GUI.Caller = this;
		event.GUI.Element = 0;
		event.GUI.Type = EGET_TAB_CHANGED;
		Parent->OnEvent(event);
	}

	if (ScrollControl) {
		CurrentScrollTabIndex = calculateScrollIndexFromActive();
		recalculateScrollBar();
	}

	return true;
}

void CGUITabControl::setVisibleTab(s32 idx)
{
	for (u32 i = 0; i < Tabs.size(); ++i)
		if (Tabs[i])
			Tabs[i]->setVisible((s32)i == idx);
}

bool CGUITabControl::setActiveTab(IGUITab *tab)
{
	return setActiveTab(getTabIndex(tab));
}

s32 CGUITabControl::getTabIndex(const IGUIElement *tab) const
{
	for (u32 i = 0; i < Tabs.size(); ++i)
		if (Tabs[i] == tab)
			return (s32)i;

	return -1;
}

//! Update the position of the element, decides scroll button status
void CGUITabControl::updateAbsolutePosition()
{
	IGUIElement::updateAbsolutePosition();
	recalculateScrollBar();
}

} // end namespace gui
