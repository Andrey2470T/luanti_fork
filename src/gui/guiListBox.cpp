// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "guiListBox.h"

#include "GUISkin.h"
#include "IGUIEnvironment.h"
#include "IGUISpriteBank.h"
#include "client/render/rendersystem.h"
#include "client/ui/text_sprite.h"
#include "guiScrollBar.h"
#include <Core/TimeCounter.h>
#include <Utils/String.h>
#include "client/ui/sprite.h"
#include "util/enriched_string.h"

namespace gui
{

//! constructor
CGUIListBox::CGUIListBox(IGUIEnvironment *environment, IGUIElement *parent,
		s32 id, recti rectangle, bool clip,
		bool drawBack, bool moveOverSelect) :
		IGUIListBox(environment, parent, id, rectangle),
		Selected(-1),
		ItemHeight(0), ItemHeightOverride(0),
		TotalItemHeight(0), ItemsIconWidth(0), Font(0), IconBank(0),
		ScrollBar(0), selectTime(0), LastKeyTime(0), Selecting(false), DrawBack(drawBack),
        MoveOverSelect(moveOverSelect), AutoScroll(true), HighlightWhenNotFocused(true),
        listBoxBank(std::make_unique<UISpriteBank>(environment->getRenderSystem(),
            environment->getResourceCache()))
{
	GUISkin *skin = Environment->getSkin();

    ScrollBar = new GUIScrollBar(Environment, this, -1, recti(0, 0, 1, 1), false, false);
	ScrollBar->setSubElement(true);
	ScrollBar->setTabStop(false);
    ScrollBar->setAlignment(EGUIA_LOWERRIGHT, EGUIA_LOWERRIGHT, GUIAlignment::UpperLeft, EGUIA_LOWERRIGHT);
	ScrollBar->setVisible(false);
	ScrollBar->setPos(0);

	updateScrollBarSize(skin->getSize(EGDS_SCROLLBAR_SIZE));

	setNotClipped(!clip);

	// this element can be tabbed to
	setTabStop(true);
	setTabOrder(-1);

	updateAbsolutePosition();
}

//! destructor
CGUIListBox::~CGUIListBox()
{
	if (ScrollBar)
		ScrollBar->drop();

	if (IconBank)
		IconBank->drop();
}

//! returns amount of list items
u32 CGUIListBox::getItemCount() const
{
	return Items.size();
}

//! returns string of a list item. the may be a value from 0 to itemCount-1
const wchar_t *CGUIListBox::getListItem(u32 id) const
{
	if (id >= Items.size())
		return 0;

	return Items[id].Text.c_str();
}

//! Returns the icon of an item
s32 CGUIListBox::getIcon(u32 id) const
{
	if (id >= Items.size())
		return -1;

	return Items[id].Icon;
}

//! adds a list item, returns id of item
u32 CGUIListBox::addItem(const wchar_t *text)
{
	return addItem(text, -1);
}

//! adds a list item, returns id of item
void CGUIListBox::removeItem(u32 id)
{
	if (id >= Items.size())
		return;

	if ((u32)Selected == id) {
		Selected = -1;
	} else if ((u32)Selected > id) {
		Selected -= 1;
		selectTime = core::TimeCounter::getRealTime();
	}

    Items.erase(Items.begin()+id);
    Rebuild = true;

	recalculateItemHeight();
}

s32 CGUIListBox::getItemAt(s32 xpos, s32 ypos) const
{
	if (xpos < AbsoluteRect.ULC.X || xpos >= AbsoluteRect.LRC.X || ypos < AbsoluteRect.ULC.Y || ypos >= AbsoluteRect.LRC.Y)
		return -1;

	if (ItemHeight == 0)
		return -1;

	s32 item = ((ypos - AbsoluteRect.ULC.Y - 1) + ScrollBar->getPos()) / ItemHeight;
	if (item < 0 || item >= (s32)Items.size())
		return -1;

	return item;
}

//! clears the list
void CGUIListBox::clear()
{
	Items.clear();
	ItemsIconWidth = 0;
	Selected = -1;

	ScrollBar->setPos(0);

	recalculateItemHeight();
}

void CGUIListBox::recalculateItemHeight()
{
	GUISkin *skin = Environment->getSkin();

	if (Font != skin->getFont()) {
		Font = skin->getFont();
        if (0 == ItemHeightOverride) {
			ItemHeight = 0;
            Rebuild = true;
        }

		if (Font) {
            if (0 == ItemHeightOverride) {
                ItemHeight = Font->getFontHeight() + 4;
                Rebuild = true;
            }
		}
	}

	TotalItemHeight = ItemHeight * Items.size();
	ScrollBar->setMax(std::max(0, TotalItemHeight - AbsoluteRect.getHeight()));
	s32 minItemHeight = ItemHeight > 0 ? ItemHeight : 1;
	ScrollBar->setSmallStep(minItemHeight);
	ScrollBar->setLargeStep(2 * minItemHeight);

	if (TotalItemHeight <= AbsoluteRect.getHeight())
		ScrollBar->setVisible(false);
	else
		ScrollBar->setVisible(true);
}

//! returns id of selected item. returns -1 if no item is selected.
s32 CGUIListBox::getSelected() const
{
	return Selected;
}

//! sets the selected item. Set this to -1 if no item should be selected
void CGUIListBox::setSelected(s32 id)
{
	if ((u32)id >= Items.size())
		Selected = -1;
	else
		Selected = id;

	selectTime = core::TimeCounter::getRealTime();

	recalculateScrollPos();
}

//! sets the selected item. Set this to -1 if no item should be selected
void CGUIListBox::setSelected(const wchar_t *item)
{
	s32 index = -1;

	if (item) {
		for (index = 0; index < (s32)Items.size(); ++index) {
			if (Items[index].Text == item)
				break;
		}
	}
	setSelected(index);
}

//! called if an event happened.
bool CGUIListBox::OnEvent(const core::Event &event)
{
	if (isEnabled()) {
		switch (event.Type) {
		case EET_KEY_INPUT_EVENT:
			if (event.KeyInput.PressedDown &&
					(event.KeyInput.Key == core::KEY_DOWN ||
							event.KeyInput.Key == core::KEY_UP ||
							event.KeyInput.Key == core::KEY_HOME ||
							event.KeyInput.Key == core::KEY_END ||
							event.KeyInput.Key == core::KEY_NEXT ||
							event.KeyInput.Key == core::KEY_PRIOR)) {
				s32 oldSelected = Selected;
				switch (event.KeyInput.Key) {
				case core::KEY_DOWN:
					Selected += 1;
					break;
				case core::KEY_UP:
					Selected -= 1;
					break;
				case core::KEY_HOME:
					Selected = 0;
					break;
				case core::KEY_END:
					Selected = (s32)Items.size() - 1;
					break;
				case core::KEY_NEXT:
					Selected += AbsoluteRect.getHeight() / ItemHeight;
					break;
				case core::KEY_PRIOR:
					Selected -= AbsoluteRect.getHeight() / ItemHeight;
					break;
				default:
					break;
				}
				if (Selected < 0)
					Selected = 0;
				if (Selected >= (s32)Items.size())
					Selected = Items.size() - 1; // will set Selected to -1 for empty listboxes which is correct

				recalculateScrollPos();

				// post the news

				if (oldSelected != Selected && Parent && !Selecting && !MoveOverSelect) {
					core::Event e;
					e.Type = EET_GUI_EVENT;
                    e.GUI.Caller = this;
					e.GUI.Element = 0;
					e.GUI.Type = EGET_LISTBOX_CHANGED;
					Parent->OnEvent(e);
				}

				return true;
			} else if (!event.KeyInput.PressedDown && (event.KeyInput.Key == core::KEY_RETURN || event.KeyInput.Key == core::KEY_SPACE)) {
				if (Parent) {
					core::Event e;
					e.Type = EET_GUI_EVENT;
                    e.GUI.Caller = this;
					e.GUI.Element = 0;
					e.GUI.Type = EGET_LISTBOX_SELECTED_AGAIN;
					Parent->OnEvent(e);
				}
				return true;
			} else if (event.KeyInput.Key == core::KEY_TAB) {
				return false;
			} else if (event.KeyInput.PressedDown && event.KeyInput.Char) {
				// change selection based on text as it is typed.
				u32 now = core::TimeCounter::getRealTime();

				if (now - LastKeyTime < 500) {
					// add to key buffer if it isn't a key repeat
					if (!(KeyBuffer.size() == 1 && KeyBuffer[0] == event.KeyInput.Char)) {
						KeyBuffer += L" ";
						KeyBuffer[KeyBuffer.size() - 1] = event.KeyInput.Char;
					}
				} else {
					KeyBuffer = L" ";
					KeyBuffer[0] = event.KeyInput.Char;
				}
				LastKeyTime = now;

				// find the selected item, starting at the current selection
				s32 start = Selected;
				// dont change selection if the key buffer matches the current item
				if (Selected > -1 && KeyBuffer.size() > 1) {
					if (Items[Selected].Text.size() >= KeyBuffer.size() &&
                        utils::equal_ignore_case<wchar_t>(KeyBuffer, Items[Selected].Text.substr(0, KeyBuffer.size())))
						return true;
				}

				s32 current;
				for (current = start + 1; current < (s32)Items.size(); ++current) {
					if (Items[current].Text.size() >= KeyBuffer.size()) {
                        if (utils::equal_ignore_case<wchar_t>(KeyBuffer, Items[current].Text.substr(0, KeyBuffer.size()))) {
							if (Parent && Selected != current && !Selecting && !MoveOverSelect) {
								core::Event e;
								e.Type = EET_GUI_EVENT;
                                e.GUI.Caller = this;
								e.GUI.Element = 0;
								e.GUI.Type = EGET_LISTBOX_CHANGED;
								Parent->OnEvent(e);
							}
							setSelected(current);
							return true;
						}
					}
				}
				for (current = 0; current <= start; ++current) {
					if (Items[current].Text.size() >= KeyBuffer.size()) {
                        if (utils::equal_ignore_case<wchar_t>(KeyBuffer, Items[current].Text.substr(0, KeyBuffer.size()))) {
							if (Parent && Selected != current && !Selecting && !MoveOverSelect) {
								Selected = current;
								core::Event e;
								e.Type = EET_GUI_EVENT;
                                e.GUI.Caller = this;
								e.GUI.Element = 0;
								e.GUI.Type = EGET_LISTBOX_CHANGED;
								Parent->OnEvent(e);
							}
							setSelected(current);
							return true;
						}
					}
				}

				return true;
			}
			break;

		case EET_GUI_EVENT:
			switch (event.GUI.Type) {
            case EGET_SCROLL_BAR_CHANGED:
                if (event.GUI.Caller == ScrollBar)
					return true;
				break;
            case EGET_ELEMENT_FOCUS_LOST: {
                if (event.GUI.Caller == this)
                    Selecting = false;
			}
			default:
				break;
			}
			break;

		case EET_MOUSE_INPUT_EVENT: {
			v2i p(event.MouseInput.X, event.MouseInput.Y);

			switch (event.MouseInput.Type) {
			case EMIE_MOUSE_WHEEL:
				ScrollBar->setPos(ScrollBar->getPos() + (event.MouseInput.WheelDelta < 0 ? -1 : 1) * -ItemHeight / 2);
				return true;

			case EMIE_LMOUSE_PRESSED_DOWN: {
				Selecting = true;
				return true;
			}

			case EMIE_LMOUSE_LEFT_UP: {
				Selecting = false;

				if (isPointInside(p))
					selectNew(event.MouseInput.Y);

				return true;
			}

			case EMIE_MOUSE_MOVED:
				if (Selecting || MoveOverSelect) {
					if (isPointInside(p)) {
						selectNew(event.MouseInput.Y, true);
						return true;
					}
				}
			default:
				break;
			}
		} break;
		default:
			break;
		}
	}

	return IGUIElement::OnEvent(event);
}

void CGUIListBox::selectNew(s32 ypos, bool onlyHover)
{
	u32 now = core::TimeCounter::getRealTime();
	s32 oldSelected = Selected;

	Selected = getItemAt(AbsoluteRect.ULC.X, ypos);
	if (Selected < 0 && !Items.empty())
		Selected = 0;

	recalculateScrollPos();

    EGUI_EVENT_TYPE eventType = (Selected == oldSelected && now < selectTime + 500) ? EGET_LISTBOX_SELECTED_AGAIN : EGET_LISTBOX_CHANGED;
	selectTime = now;
	// post the news
	if (Parent && !onlyHover) {
		core::Event event;
		event.Type = EET_GUI_EVENT;
        event.GUI.Caller = this;
		event.GUI.Element = 0;
		event.GUI.Type = eventType;
		Parent->OnEvent(event);
	}
}

//! Update the position and size of the listbox, and update the scrollbar
void CGUIListBox::updateAbsolutePosition()
{
	IGUIElement::updateAbsolutePosition();

	recalculateItemHeight();
}

void CGUIListBox::updateMesh()
{
    if (ScrollBar->isVisible() && LastScrollPos != ScrollBar->getPos()) {
        LastScrollPos = ScrollBar->getPos();
        Rebuild = true;
    }

    if (!Rebuild)
        return;

    recalculateItemHeight(); // if the font changed

    GUISkin *skin = Environment->getSkin();
    updateScrollBarSize(skin->getSize(EGDS_SCROLLBAR_SIZE));

    listBoxBank->clear();

    recti *clipRect = 0;

    // draw background
    recti frameRect(AbsoluteRect);

    // draw items
    recti clientClip(AbsoluteRect);
    clientClip.ULC.Y += 1;
    clientClip.ULC.X += 1;
    if (ScrollBar->isVisible())
        clientClip.LRC.X -= ScrollBar->getRelativePosition().getWidth();
    clientClip.LRC.Y -= 1;
    clientClip.clipAgainst(AbsoluteClippingRect);

    listBoxBank->addSprite({{rectf(), {}}}, 0);
    skin->add3DSunkenPane(listBoxBank->getSprite(0), skin->getColor(EGDC_3D_HIGH_LIGHT), true,
            DrawBack, toRectT<f32>(frameRect));
    listBoxBank->getSprite(0)->setClipRect(clientClip);
    listBoxBank->getSprite(0)->rebuildMesh();

    if (clipRect)
        clientClip.clipAgainst(*clipRect);

    frameRect = AbsoluteRect;
    frameRect.ULC.X += 1;
    if (ScrollBar->isVisible())
        frameRect.LRC.X -= ScrollBar->getRelativePosition().getWidth();

    frameRect.LRC.Y = AbsoluteRect.ULC.Y + ItemHeight;

    frameRect.ULC.Y -= ScrollBar->getPos();
    frameRect.LRC.Y -= ScrollBar->getPos();

    bool hl = (HighlightWhenNotFocused || Environment->hasFocus(this) || Environment->hasFocus(ScrollBar));

    auto itemColor = skin->getColor(EGDC_HIGH_LIGHT);
    for (s32 i = 0; i < (s32)Items.size(); ++i) {
        if (frameRect.LRC.Y >= AbsoluteRect.ULC.Y &&
                frameRect.ULC.Y <= AbsoluteRect.LRC.Y) {
            if (i == Selected && hl)
                listBoxBank->addSprite({{toRectT<f32>(frameRect), {itemColor, itemColor, itemColor, itemColor}}}, &clientClip);

            recti textRect = frameRect;
            textRect.ULC.X += 3;

            if (Font) {
                if (IconBank && (Items[i].Icon > -1)) {
                    v2i iconPos = textRect.ULC;
                    iconPos.Y += textRect.getHeight() / 2;
                    iconPos.X += ItemsIconWidth / 2;

                    if (i == Selected && hl) {
                        IconBank->update2DSprite((u32)Items[i].Icon, iconPos, &clientClip,
                                hasItemOverrideColor(i, EGUI_LBC_ICON_HIGHLIGHT) ? getItemOverrideColor(i, EGUI_LBC_ICON_HIGHLIGHT) : getItemDefaultColor(EGUI_LBC_ICON_HIGHLIGHT),
                                selectTime, core::TimeCounter::getRealTime(), false, true);
                    } else {
                        IconBank->update2DSprite((u32)Items[i].Icon, iconPos, &clientClip,
                                hasItemOverrideColor(i, EGUI_LBC_ICON) ? getItemOverrideColor(i, EGUI_LBC_ICON) : getItemDefaultColor(EGUI_LBC_ICON),
                                0, (i == Selected) ? core::TimeCounter::getRealTime() : 0, false, true);
                    }
                }

                textRect.ULC.X += ItemsIconWidth + 3;

                img::color8 textColor;
                if (i == Selected && hl) {
                    textColor = hasItemOverrideColor(i, EGUI_LBC_TEXT_HIGHLIGHT) ?
                        getItemOverrideColor(i, EGUI_LBC_TEXT_HIGHLIGHT) : getItemDefaultColor(EGUI_LBC_TEXT_HIGHLIGHT);
                } else {
                    textColor = hasItemOverrideColor(i, EGUI_LBC_TEXT) ?
                        getItemOverrideColor(i, EGUI_LBC_TEXT) : getItemDefaultColor(EGUI_LBC_TEXT);
                }

                auto itemText = listBoxBank->addTextSprite(Items[i].Text,
                    toRectT<f32>(textRect), textColor, &clientClip);
                itemText->setAlignment(GUIAlignment::UpperLeft, GUIAlignment::Center);
                itemText->updateBuffer(toRectT<f32>(textRect));

                textRect.ULC.X -= ItemsIconWidth + 3;
            }
        }

        frameRect.ULC.Y += ItemHeight;
        frameRect.LRC.Y += ItemHeight;
    }

    Rebuild = false;
}

//! draws the element and its children
void CGUIListBox::draw()
{
	if (!IsVisible)
		return;

    updateMesh();
    listBoxBank->drawBank();

	IGUIElement::draw();
}

//! adds an list item with an icon
u32 CGUIListBox::addItem(const wchar_t *text, s32 icon)
{
	ListItem i;
	i.Text = text;
	i.Icon = icon;

	Items.push_back(i);
	recalculateItemHeight();
	recalculateItemWidth(icon);

    Rebuild = true;
	return Items.size() - 1;
}

void CGUIListBox::setSpriteBank(IGUISpriteBank *bank)
{
	if (bank == IconBank)
		return;
	if (IconBank)
		IconBank->drop();

	IconBank = bank;
	if (IconBank)
		IconBank->grab();
}

void CGUIListBox::recalculateScrollPos()
{
	if (!AutoScroll)
		return;

	const s32 selPos = (getSelected() == -1 ? TotalItemHeight : getSelected() * ItemHeight) - ScrollBar->getPos();

	if (selPos < 0) {
		ScrollBar->setPos(ScrollBar->getPos() + selPos);
	} else if (selPos > AbsoluteRect.getHeight() - ItemHeight) {
		ScrollBar->setPos(ScrollBar->getPos() + selPos - AbsoluteRect.getHeight() + ItemHeight);
	}
}

void CGUIListBox::updateScrollBarSize(s32 size)
{
	if (size != ScrollBar->getRelativePosition().getWidth()) {
		recti r(RelativeRect.getWidth() - size, 0, RelativeRect.getWidth(), RelativeRect.getHeight());
		ScrollBar->setRelativePosition(r);
	}
}

void CGUIListBox::setAutoScrollEnabled(bool scroll)
{
	AutoScroll = scroll;
}

bool CGUIListBox::isAutoScrollEnabled() const
{
	return AutoScroll;
}

bool CGUIListBox::getSerializationLabels(EGUI_LISTBOX_COLOR colorType, std::string &useColorLabel, std::string &colorLabel) const
{
	switch (colorType) {
	case EGUI_LBC_TEXT:
		useColorLabel = "UseColText";
		colorLabel = "ColText";
		break;
	case EGUI_LBC_TEXT_HIGHLIGHT:
		useColorLabel = "UseColTextHl";
		colorLabel = "ColTextHl";
		break;
	case EGUI_LBC_ICON:
		useColorLabel = "UseColIcon";
		colorLabel = "ColIcon";
		break;
	case EGUI_LBC_ICON_HIGHLIGHT:
		useColorLabel = "UseColIconHl";
		colorLabel = "ColIconHl";
		break;
	default:
		return false;
	}
	return true;
}

void CGUIListBox::recalculateItemWidth(s32 icon)
{
	if (IconBank && icon > -1 &&
			IconBank->getSprites().size() > (u32)icon &&
			IconBank->getSprites()[(u32)icon].Frames.size()) {
		u32 rno = IconBank->getSprites()[(u32)icon].Frames[0].rectNumber;
		if (IconBank->getPositions().size() > rno) {
			const s32 w = IconBank->getPositions()[rno].getWidth();
			if (w > ItemsIconWidth)
				ItemsIconWidth = w;
		}
	}
}

void CGUIListBox::setItem(u32 index, const wchar_t *text, s32 icon)
{
	if (index >= Items.size())
		return;

	Items[index].Text = text;
	Items[index].Icon = icon;

	recalculateItemHeight();
	recalculateItemWidth(icon);

    Rebuild = true;
}

//! Insert the item at the given index
//! Return the index on success or -1 on failure.
s32 CGUIListBox::insertItem(u32 index, const wchar_t *text, s32 icon)
{
	ListItem i;
	i.Text = text;
	i.Icon = icon;

    Items.insert(Items.begin()+index, i);
	recalculateItemHeight();
	recalculateItemWidth(icon);

    Rebuild = true;

	return index;
}

void CGUIListBox::swapItems(u32 index1, u32 index2)
{
	if (index1 >= Items.size() || index2 >= Items.size())
		return;

	ListItem dummmy = Items[index1];
	Items[index1] = Items[index2];
	Items[index2] = dummmy;

    Rebuild = true;
}

void CGUIListBox::setItemOverrideColor(u32 index, img::color8 color)
{
    for (u32 c = 0; c < (u8)EGUI_LBC_COUNT; ++c) {
		Items[index].OverrideColors[c].Use = true;
		Items[index].OverrideColors[c].Color = color;
	}
    Rebuild = true;
}

void CGUIListBox::setItemOverrideColor(u32 index, EGUI_LISTBOX_COLOR colorType, img::color8 color)
{
    if (index >= (u32)Items.size() || (u8)colorType < 0 || colorType >= EGUI_LBC_COUNT)
		return;

    Items[index].OverrideColors[(u8)colorType].Use = true;
    Items[index].OverrideColors[(u8)colorType].Color = color;

    Rebuild = true;
}

void CGUIListBox::clearItemOverrideColor(u32 index)
{
    for (u32 c = 0; c < (u32)EGUI_LBC_COUNT; ++c) {
		Items[index].OverrideColors[c].Use = false;
	}

    Rebuild = true;
}

void CGUIListBox::clearItemOverrideColor(u32 index, EGUI_LISTBOX_COLOR colorType)
{
    if (index >= (u32)Items.size() || (u8)colorType < 0 || colorType >= EGUI_LBC_COUNT)
		return;

    Items[index].OverrideColors[(u8)colorType].Use = false;

    Rebuild = true;
}

bool CGUIListBox::hasItemOverrideColor(u32 index, EGUI_LISTBOX_COLOR colorType) const
{
    if (index >= (u32)Items.size() || (u8)colorType < 0 || colorType >= EGUI_LBC_COUNT)
		return false;

    return Items[index].OverrideColors[(u8)colorType].Use;
}

img::color8 CGUIListBox::getItemOverrideColor(u32 index, EGUI_LISTBOX_COLOR colorType) const
{
    if ((u32)index >= (u32)Items.size() || (u8)colorType < 0 || colorType >= EGUI_LBC_COUNT)
		return img::color8();

    return Items[index].OverrideColors[(u8)colorType].Color;
}

img::color8 CGUIListBox::getItemDefaultColor(EGUI_LISTBOX_COLOR colorType) const
{
	GUISkin *skin = Environment->getSkin();
	if (!skin)
		return img::color8();

	switch (colorType) {
	case EGUI_LBC_TEXT:
		return skin->getColor(EGDC_BUTTON_TEXT);
	case EGUI_LBC_TEXT_HIGHLIGHT:
		return skin->getColor(EGDC_HIGH_LIGHT_TEXT);
	case EGUI_LBC_ICON:
		return skin->getColor(EGDC_ICON);
	case EGUI_LBC_ICON_HIGHLIGHT:
		return skin->getColor(EGDC_ICON_HIGH_LIGHT);
	default:
		return img::color8();
	}
}

//! set global itemHeight
void CGUIListBox::setItemHeight(s32 height)
{
	ItemHeight = height;
	ItemHeightOverride = 1;

    Rebuild = true;
}

//! Sets whether to draw the background
void CGUIListBox::setDrawBackground(bool draw)
{
    if (draw != DrawBack) Rebuild = true;
	DrawBack = draw;
}

//! Access the vertical scrollbar
IGUIElement *CGUIListBox::getVerticalScrollBar() const
{
	return ScrollBar;
}

} // end namespace gui
