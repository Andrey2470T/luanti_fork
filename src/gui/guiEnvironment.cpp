
// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "guiEnvironment.h"

#include "GUISkin.h"
#include "guiButton.h"
#include "CGUIScrollBar.h"
#include <Render/TTFont.h>
#include "CGUISpriteBank.h"
#include "CGUIImage.h"
#include "CGUICheckBox.h"
#include "CGUIListBox.h"
#include "CGUIImageList.h"
#include "CGUIFileOpenDialog.h"
#include "CGUIStaticText.h"
#include "CGUIEditBox.h"
#include "CGUITabControl.h"
#include "CGUIComboBox.h"
#include <Main/TimeCounter.h>
#include "client/render/rendersystem.h"
#include "client/ui/glyph_atlas.h"
#include "client/media/resource.h"

namespace gui
{

const std::string CGUIEnvironment::DefaultFontName = "#DefaultFont";

//! constructor
CGUIEnvironment::CGUIEnvironment(RenderSystem *rndsys, v2u wnd_size, ResourceCache *rescache) :
        IGUIElement(EGUIET_ROOT, 0, 0, 0, recti(0, 0, wnd_size.X, wnd_size.Y)),
        Hovered(0), HoveredNoSubelement(0), Focus(0), LastHoveredMousePos(0, 0),
        UserReceiver(0), FocusFlags((u8)EFF_SET_ON_LMOUSE_DOWN | (u8)EFF_SET_ON_TAB),
        RndSys(rndsys), ResCache(rescache)
{
	loadBuiltInFont();

    GUISkin *skin = createSkin();
	setSkin(skin);

	// set tooltip default
	ToolTip.LastTime = 0;
	ToolTip.EnterTime = 0;
	ToolTip.LaunchTime = 1000;
	ToolTip.RelaunchTime = 500;
	ToolTip.Element = 0;

	// environment is root tab group
	Environment = this;
	setTabGroup(true);
}

//! destructor
CGUIEnvironment::~CGUIEnvironment()
{
	clearDeletionQueue();

	if (HoveredNoSubelement && HoveredNoSubelement != this) {
		HoveredNoSubelement->drop();
		HoveredNoSubelement = 0;
	}

	if (Hovered && Hovered != this) {
		Hovered->drop();
		Hovered = 0;
	}

	if (Focus) {
		Focus->drop();
		Focus = 0;
	}

	if (ToolTip.Element) {
		ToolTip.Element->drop();
		ToolTip.Element = 0;
	}

	u32 i;

	// delete all sprite banks
	for (i = 0; i < Banks.size(); ++i)
		if (Banks[i].Bank)
			Banks[i].Bank->drop();
}

//! draws all gui elements
void CGUIEnvironment::drawAll(bool useScreenSize)
{
    if (useScreenSize) {
        v2u dim(RndSys->getWindowSize());
        if ((u32)AbsoluteRect.LRC.X != dim.X ||
            (u32)AbsoluteRect.ULC.X != 0 ||
            (u32)AbsoluteRect.LRC.Y != dim.Y ||
            AbsoluteRect.ULC.Y != 0) {
            setRelativePosition(recti(0, 0, dim.X, dim.Y));
		}
	}

	// make sure tooltip is always on top
	if (ToolTip.Element)
		bringToFront(ToolTip.Element);

	draw();
	OnPostRender(main::TimeCounter::getRealTime());

	clearDeletionQueue();
}

//! sets the focus to an element
bool CGUIEnvironment::setFocus(IGUIElement *element)
{
	if (Focus == element) {
		return false;
	}

	// GUI Environment should just reset the focus to 0
	if (element == this)
		element = 0;

	// stop element from being deleted
	if (element)
		element->grab();

	// focus may change or be removed in this call
	IGUIElement *currentFocus = 0;
	if (Focus) {
		currentFocus = Focus;
		currentFocus->grab();
		main::Event e;
		e.Type = EET_GUI_EVENT;
        e.GUI.Caller = Focus->getID();
        e.GUI.Element = element->getID();
		e.GUI.Type = EGET_ELEMENT_FOCUS_LOST;
		if (Focus->OnEvent(e)) {
			if (element)
				element->drop();
			currentFocus->drop();
			return false;
		}
		currentFocus->drop();
		currentFocus = 0;
	}

	if (element) {
		currentFocus = Focus;
		if (currentFocus)
			currentFocus->grab();

		// send focused event
		main::Event e;
		e.Type = EET_GUI_EVENT;
        e.GUI.Caller = element->getID();
        e.GUI.Element = Focus->getID();
		e.GUI.Type = EGET_ELEMENT_FOCUSED;
		if (element->OnEvent(e)) {
			if (element)
				element->drop();
			if (currentFocus)
				currentFocus->drop();
			return false;
		}
	}

	if (currentFocus)
		currentFocus->drop();

	if (Focus)
		Focus->drop();

	// element is the new focus so it doesn't have to be dropped
	Focus = element;

	return true;
}

//! returns the element with the focus
IGUIElement *CGUIEnvironment::getFocus() const
{
	return Focus;
}

//! returns the element last known to be under the mouse cursor
IGUIElement *CGUIEnvironment::getHovered() const
{
	return Hovered;
}

//! removes the focus from an element
bool CGUIEnvironment::removeFocus(IGUIElement *element)
{
	if (Focus && Focus == element) {
		main::Event e;
		e.Type = EET_GUI_EVENT;
        e.GUI.Caller = Focus->getID();
        e.GUI.Element = std::nullopt;
		e.GUI.Type = EGET_ELEMENT_FOCUS_LOST;
		if (Focus->OnEvent(e)) {
			return false;
		}
	}
	if (Focus) {
		Focus->drop();
		Focus = 0;
	}

	return true;
}

//! Returns whether the element has focus
bool CGUIEnvironment::hasFocus(const IGUIElement *element, bool checkSubElements) const
{
	if (element == Focus)
		return true;

	if (!checkSubElements || !element)
		return false;

	IGUIElement *f = Focus;
	while (f && f->isSubElement()) {
		f = f->getParent();
		if (f == element)
			return true;
	}
	return false;
}

//! clear all GUI elements
void CGUIEnvironment::clear()
{
	// Remove the focus
	if (Focus) {
		Focus->drop();
		Focus = 0;
	}

	if (Hovered && Hovered != this) {
		Hovered->drop();
		Hovered = 0;
	}
	if (HoveredNoSubelement && HoveredNoSubelement != this) {
		HoveredNoSubelement->drop();
		HoveredNoSubelement = 0;
	}

	getRootGUIElement()->removeAllChildren();
}

//! called by ui if an event happened.
bool CGUIEnvironment::OnEvent(const main::Event &event)
{

	bool ret = false;
    if (UserReceiver && (event.Type != EET_MOUSE_INPUT_EVENT) && (event.Type != EET_KEY_INPUT_EVENT) && (event.Type != EET_GUI_EVENT || event.GUI.Caller != getID())) {
		ret = UserReceiver->OnEvent(event);
	}

	return ret;
}

//
void CGUIEnvironment::OnPostRender(u32 time)
{
	// launch tooltip
	if (ToolTip.Element == 0 &&
			HoveredNoSubelement && HoveredNoSubelement != this &&
			(time - ToolTip.EnterTime >= ToolTip.LaunchTime || (time - ToolTip.LastTime >= ToolTip.RelaunchTime && time - ToolTip.LastTime < ToolTip.LaunchTime)) &&
			HoveredNoSubelement->getToolTipText().size() &&
			getSkin() &&
			getSkin()->getFont(EGDF_TOOLTIP)) {
		recti pos;

		pos.ULC = LastHoveredMousePos;
        v2u dim = getSkin()->getFont(EGDF_TOOLTIP)->getTextSize(HoveredNoSubelement->getToolTipText().c_str());
        dim.X += getSkin()->getSize(EGDS_TEXT_DISTANCE_X) * 2;
        dim.Y += getSkin()->getSize(EGDS_TEXT_DISTANCE_Y) * 2;

        pos.ULC.Y -= dim.Y + 1;
        pos.LRC.Y = pos.ULC.Y + dim.Y - 1;
        pos.LRC.X = pos.ULC.X + dim.X;

		pos.constrainTo(getAbsolutePosition());

		ToolTip.Element = addStaticText(HoveredNoSubelement->getToolTipText().c_str(), pos, true, true, this, -1, true);
		ToolTip.Element->setOverrideColor(getSkin()->getColor(EGDC_TOOLTIP));
		ToolTip.Element->setBackgroundColor(getSkin()->getColor(EGDC_TOOLTIP_BACKGROUND));
		ToolTip.Element->setOverrideFont(getSkin()->getFont(EGDF_TOOLTIP));
		ToolTip.Element->setSubElement(true);
		ToolTip.Element->grab();

		s32 textHeight = ToolTip.Element->getTextHeight();
		pos = ToolTip.Element->getRelativePosition();
		pos.LRC.Y = pos.ULC.Y + textHeight;
		ToolTip.Element->setRelativePosition(pos);
	}

	if (ToolTip.Element && ToolTip.Element->isVisible()) { // (isVisible() check only because we might use visibility for ToolTip one day)
		ToolTip.LastTime = time;

		// got invisible or removed in the meantime?
		if (!HoveredNoSubelement ||
				!HoveredNoSubelement->isVisible() ||
				!HoveredNoSubelement->getParent()) // got invisible or removed in the meantime?
		{
			ToolTip.Element->remove();
			ToolTip.Element->drop();
			ToolTip.Element = 0;
		}
	}

	IGUIElement::OnPostRender(time);
}

void CGUIEnvironment::addToDeletionQueue(IGUIElement *element)
{
	if (!element)
		return;

	element->grab();
	DeletionQueue.push_back(element);
}

void CGUIEnvironment::clearDeletionQueue()
{
	if (DeletionQueue.empty())
		return;

	for (u32 i = 0; i < DeletionQueue.size(); ++i) {
		DeletionQueue[i]->remove();
		DeletionQueue[i]->drop();
	}

	DeletionQueue.clear();
}

//
void CGUIEnvironment::updateHoveredElement(v2i mousePos)
{
	IGUIElement *lastHovered = Hovered;
	IGUIElement *lastHoveredNoSubelement = HoveredNoSubelement;
	LastHoveredMousePos = mousePos;

	Hovered = getElementFromPoint(mousePos);

	if (ToolTip.Element && Hovered == ToolTip.Element) {
		// When the mouse is over the ToolTip we remove that so it will be re-created at a new position.
		// Note that ToolTip.EnterTime does not get changed here, so it will be re-created at once.
		ToolTip.Element->remove();
		ToolTip.Element->drop();
		ToolTip.Element = 0;

		// Get the real Hovered
		Hovered = getElementFromPoint(mousePos);
	}

	// for tooltips we want the element itself and not some of it's subelements
	HoveredNoSubelement = Hovered;
	while (HoveredNoSubelement && HoveredNoSubelement->isSubElement()) {
		HoveredNoSubelement = HoveredNoSubelement->getParent();
	}

	if (Hovered && Hovered != this)
		Hovered->grab();
	if (HoveredNoSubelement && HoveredNoSubelement != this)
		HoveredNoSubelement->grab();

	if (Hovered != lastHovered) {
		main::Event event;
		event.Type = EET_GUI_EVENT;

		if (lastHovered) {
            event.GUI.Caller = lastHovered->getID();
            event.GUI.Element = std::nullopt;
			event.GUI.Type = EGET_ELEMENT_LEFT;
			lastHovered->OnEvent(event);
		}

		if (Hovered) {
            event.GUI.Caller = Hovered->getID();
            event.GUI.Element = Hovered->getID();
			event.GUI.Type = EGET_ELEMENT_HOVERED;
			Hovered->OnEvent(event);
		}
	}

	if (lastHoveredNoSubelement != HoveredNoSubelement) {
		if (ToolTip.Element) {
			ToolTip.Element->remove();
			ToolTip.Element->drop();
			ToolTip.Element = 0;
		}

		if (HoveredNoSubelement) {
			u32 now = main::TimeCounter::getRealTime();
			ToolTip.EnterTime = now;
		}
	}

	if (lastHovered && lastHovered != this)
		lastHovered->drop();
	if (lastHoveredNoSubelement && lastHoveredNoSubelement != this)
		lastHoveredNoSubelement->drop();
}

//! This sets a new event receiver for gui events. Usually you do not have to
//! use this method, it is used by the internal engine.
void CGUIEnvironment::setUserEventReceiver(main::IEventReceiver *evr)
{
	UserReceiver = evr;
}

//! posts an input event to the environment
bool CGUIEnvironment::postEventFromUser(const main::Event &event)
{
	switch (event.Type) {
	case EET_GUI_EVENT: {
		// hey, why is the user sending gui events..?
	}

	break;
	case EET_MOUSE_INPUT_EVENT:

		updateHoveredElement(v2i(event.MouseInput.X, event.MouseInput.Y));

		if (Hovered != Focus) {
			IGUIElement *focusCandidate = Hovered;

			// Only allow enabled elements to be focused (unless EFF_CAN_FOCUS_DISABLED is set)
            if (Hovered && !Hovered->isEnabled() && !(FocusFlags & (u32)EFF_CAN_FOCUS_DISABLED))
				focusCandidate = NULL; // we still remove focus from the active element

			// Please don't merge this into a single if clause, it's easier to debug the way it is
            if (FocusFlags & (u32)EFF_SET_ON_LMOUSE_DOWN &&
					event.MouseInput.Type == EMIE_LMOUSE_PRESSED_DOWN) {
				setFocus(focusCandidate);
            } else if (FocusFlags & (u32)EFF_SET_ON_RMOUSE_DOWN &&
					   event.MouseInput.Type == EMIE_RMOUSE_PRESSED_DOWN) {
				setFocus(focusCandidate);
            } else if (FocusFlags & (u32)EFF_SET_ON_MOUSE_OVER &&
					   event.MouseInput.Type == EMIE_MOUSE_MOVED) {
				setFocus(focusCandidate);
			}
		}

		// sending input to focus
		if (Focus && Focus->OnEvent(event))
			return true;

		// focus could have died in last call
		if (!Focus && Hovered) {
			return Hovered->OnEvent(event);
		}

		break;
	case EET_KEY_INPUT_EVENT: {
		if (Focus && Focus->OnEvent(event))
			return true;

		// For keys we handle the event before changing focus to give elements the chance for catching the TAB
		// Send focus changing event
		// CAREFUL when changing - there's an identical check in CGUIModalScreen::OnEvent
        if (FocusFlags & (u32)EFF_SET_ON_TAB &&
				event.KeyInput.PressedDown &&
				event.KeyInput.Key == KEY_TAB) {
			IGUIElement *next = getNextElement(event.KeyInput.Shift, event.KeyInput.Control);
			if (next && next != Focus) {
				if (setFocus(next))
					return true;
			}
		}
	} break;
	case EET_STRING_INPUT_EVENT:
		if (Focus && Focus->OnEvent(event))
			return true;
		break;
	default:
		break;
	} // end switch

	return false;
}

//! returns the current gui skin
GUISkin *CGUIEnvironment::getSkin() const
{
    return CurrentSkin.get();
}

//! Sets a new GUI Skin
void CGUIEnvironment::setSkin(GUISkin *skin)
{
    if (CurrentSkin.get() == skin)
		return;

    CurrentSkin.reset(skin);
}

//! Creates a new GUI Skin.
/** \return Returns a pointer to the created skin.
If you no longer need the skin, you should call GUISkin::drop().
See IReferenceCounted::drop() for more information. */
GUISkin *CGUIEnvironment::createSkin()
{
    GUISkin *skin = new GUISkin(RndSys->getRenderer());

    auto font_mgr = RndSys->getFontManager();
    render::TTFont *defaultfont = font_mgr->getFontOrCreate(
        render::FontMode::MONO, render::FontStyle::NORMAL);

	IGUISpriteBank *bank = 0;
    skin->setFont(defaultfont);

	if (bitfont)
		bank = bitfont->getSpriteBank();

	skin->setSpriteBank(bank);

	return skin;
}

//! adds a button. The returned pointer must not be dropped.
IGUIButton *CGUIEnvironment::addButton(const recti &rectangle, IGUIElement *parent, s32 id, const wchar_t *text, const wchar_t *tooltiptext)
{
	IGUIButton *button = new CGUIButton(this, parent ? parent : this, id, rectangle);
	if (text)
		button->setText(text);

	if (tooltiptext)
		button->setToolTipText(tooltiptext);

	button->drop();
	return button;
}

//! adds a scrollbar. The returned pointer must not be dropped.
IGUIScrollBar *CGUIEnvironment::addScrollBar(bool horizontal, const recti &rectangle, IGUIElement *parent, s32 id)
{
	IGUIScrollBar *bar = new CGUIScrollBar(horizontal, this, parent ? parent : this, id, rectangle);
	bar->drop();
	return bar;
}

//! Adds an image element.
IGUIImage *CGUIEnvironment::addImage(img::Image *image, v2i pos,
		bool useAlphaChannel, IGUIElement *parent, s32 id, const wchar_t *text)
{
    v2i sz(0, 0);
	if (image)
        sz = v2i(image->getSize().X, image->getSize().Y);

	IGUIImage *img = new CGUIImage(this, parent ? parent : this,
			id, recti(pos, sz));

	if (text)
		img->setText(text);

	if (useAlphaChannel)
		img->setUseAlphaChannel(true);

	if (image)
		img->setImage(image);

	img->drop();
	return img;
}

//! adds an image. The returned pointer must not be dropped.
IGUIImage *CGUIEnvironment::addImage(const recti &rectangle, IGUIElement *parent, s32 id, const wchar_t *text, bool useAlphaChannel)
{
	IGUIImage *img = new CGUIImage(this, parent ? parent : this,
			id, rectangle);

	if (text)
		img->setText(text);

	if (useAlphaChannel)
		img->setUseAlphaChannel(true);

	img->drop();
	return img;
}

//! adds a checkbox
IGUICheckBox *CGUIEnvironment::addCheckBox(bool checked, const recti &rectangle, IGUIElement *parent, s32 id, const wchar_t *text)
{
	IGUICheckBox *b = new CGUICheckBox(checked, this,
			parent ? parent : this, id, rectangle);

	if (text)
		b->setText(text);

	b->drop();
	return b;
}

//! adds a list box
IGUIListBox *CGUIEnvironment::addListBox(const recti &rectangle,
		IGUIElement *parent, s32 id, bool drawBackground)
{
	IGUIListBox *b = new CGUIListBox(this, parent ? parent : this, id, rectangle,
			true, drawBackground, false);

	if (CurrentSkin && CurrentSkin->getSpriteBank()) {
		b->setSpriteBank(CurrentSkin->getSpriteBank());
	} else if (getBuiltInFont() && getBuiltInFont()->getType() == EGFT_BITMAP) {
		b->setSpriteBank(((render::TTFontBitmap *)getBuiltInFont())->getSpriteBank());
	}

	b->drop();
	return b;
}

//! adds a file open dialog. The returned pointer must not be dropped.
IGUIFileOpenDialog *CGUIEnvironment::addFileOpenDialog(const wchar_t *title,
        bool modal, IGUIElement *parent, s32 id,
        bool restoreCWD, std::string *startDir)
{
	parent = parent ? parent : this;

	if (modal)
		return nullptr;

	IGUIFileOpenDialog *d = new CGUIFileOpenDialog(title, this, parent, id,
			restoreCWD, startDir);
	d->drop();

	return d;
}

//! adds a static text. The returned pointer must not be dropped.
IGUIStaticText *CGUIEnvironment::addStaticText(const wchar_t *text,
		const recti &rectangle,
		bool border, bool wordWrap,
		IGUIElement *parent, s32 id, bool background)
{
	IGUIStaticText *d = new CGUIStaticText(text, border, this,
			parent ? parent : this, id, rectangle, background);

	d->setWordWrap(wordWrap);
	d->drop();

	return d;
}

//! Adds an edit box. The returned pointer must not be dropped.
IGUIEditBox *CGUIEnvironment::addEditBox(const wchar_t *text,
		const recti &rectangle, bool border,
		IGUIElement *parent, s32 id)
{
	IGUIEditBox *d = new CGUIEditBox(text, border, this,
			parent ? parent : this, id, rectangle);

	d->drop();
	return d;
}

//! Adds a tab control to the environment.
IGUITabControl *CGUIEnvironment::addTabControl(const recti &rectangle,
		IGUIElement *parent, bool fillbackground, bool border, s32 id)
{
	IGUITabControl *t = new CGUITabControl(this, parent ? parent : this,
			rectangle, fillbackground, border, id);
	t->drop();
	return t;
}

//! Adds tab to the environment.
IGUITab *CGUIEnvironment::addTab(const recti &rectangle,
		IGUIElement *parent, s32 id)
{
	IGUITab *t = new CGUITab(this, parent ? parent : this,
			rectangle, id);
	t->drop();
	return t;
}

//! Adds a combo box to the environment.
IGUIComboBox *CGUIEnvironment::addComboBox(const recti &rectangle,
		IGUIElement *parent, s32 id)
{
	IGUIComboBox *t = new CGUIComboBox(this, parent ? parent : this,
			id, rectangle);
	t->drop();
	return t;
}

//! returns the font
render::TTFont *CGUIEnvironment::getFont(const std::string &filename)
{
	// search existing font

	SFont f;
	f.NamedPath.setPath(filename);

	s32 index = Fonts.binary_search(f);
	if (index != -1)
		return Fonts[index].Font;

	// font doesn't exist, attempt to load it

	// does the file exist?

	if (!FileSystem->existFile(filename)) {
		os::Printer::log("Could not load font because the file does not exist", f.NamedPath.getPath(), ELL_ERROR);
		return 0;
	}

	render::TTFont *ifont = 0;
#if 0
		{
			CGUIFont* font = new CGUIFont(this, filename);
			ifont = (render::TTFont*)font;

			// load the font
            std::string directory;
			core::splitFilename(filename, &directory);
			if (!font->load(xml, directory))
			{
				font->drop();
				font  = 0;
				ifont = 0;
			}
		}
#endif

	if (!ifont) {

		CGUIFont *font = new CGUIFont(this, f.NamedPath.getPath());
		ifont = (render::TTFont *)font;
		if (!font->load(f.NamedPath.getPath())) {
			font->drop();
			return 0;
		}
	}

	// add to fonts.

	f.Font = ifont;
	Fonts.push_back(f);

	return ifont;
}

//! add an externally loaded font
render::TTFont *CGUIEnvironment::addFont(const std::string &name, render::TTFont *font)
{
	if (font) {
		SFont f;
		f.NamedPath.setPath(name);
		s32 index = Fonts.binary_search(f);
		if (index != -1)
			return Fonts[index].Font;
		f.Font = font;
		Fonts.push_back(f);
		font->grab();
	}
	return font;
}

//! remove loaded font
void CGUIEnvironment::removeFont(render::TTFont *font)
{
	if (!font)
		return;
	for (u32 i = 0; i < Fonts.size(); ++i) {
		if (Fonts[i].Font == font) {
			Fonts[i].Font->drop();
			Fonts.erase(i);
			return;
		}
	}
}

//! returns default font
render::TTFont *CGUIEnvironment::getBuiltInFont() const
{
	if (Fonts.empty())
		return 0;

	return Fonts[0].Font;
}

IGUISpriteBank *CGUIEnvironment::getSpriteBank(const std::string &filename)
{
	// search for the file name

	SSpriteBank b;
	b.NamedPath.setPath(filename);

	s32 index = Banks.binary_search(b);
	if (index != -1)
		return Banks[index].Bank;

	// we don't have this sprite bank, we should load it
	if (!FileSystem->existFile(b.NamedPath.getPath())) {
		if (filename != DefaultFontName) {
			os::Printer::log("Could not load sprite bank because the file does not exist", b.NamedPath.getPath(), ELL_DEBUG);
		}
		return 0;
	}

	// todo: load it!

	return 0;
}

IGUISpriteBank *CGUIEnvironment::addEmptySpriteBank(const std::string &name)
{
	// no duplicate names allowed

	SSpriteBank b;
	b.NamedPath.setPath(name);

	const s32 index = Banks.binary_search(b);
	if (index != -1)
		return 0;

	// create a new sprite bank

	b.Bank = new CGUISpriteBank(this);
	Banks.push_back(b);

	return b.Bank;
}

//! Creates the image list from the given texture.
IGUIImageList *CGUIEnvironment::createImageList(img::Image *texture,
        v2i imageSize, bool useAlphaChannel)
{
	CGUIImageList *imageList = new CGUIImageList(Driver);
	if (!imageList->createImageList(texture, imageSize, useAlphaChannel)) {
		imageList->drop();
		return 0;
	}

	return imageList;
}

//! Returns the root gui element.
IGUIElement *CGUIEnvironment::getRootGUIElement()
{
	return this;
}

//! Returns the next element in the tab group starting at the focused element
IGUIElement *CGUIEnvironment::getNextElement(bool reverse, bool group)
{
	// start the search at the root of the current tab group
	IGUIElement *startPos = Focus ? Focus->getTabGroup() : 0;
	s32 startOrder = -1;

	// if we're searching for a group
	if (group && startPos) {
		startOrder = startPos->getTabOrder();
	} else if (!group && Focus && !Focus->isTabGroup()) {
		startOrder = Focus->getTabOrder();
		if (startOrder == -1) {
			// this element is not part of the tab cycle,
			// but its parent might be...
			IGUIElement *el = Focus;
			while (el->getParent() && startOrder == -1) {
				el = el->getParent();
				startOrder = el->getTabOrder();
			}
		}
	}

	if (group || !startPos)
		startPos = this; // start at the root

	// find the element
	IGUIElement *closest = 0;
	IGUIElement *first = 0;
	startPos->getNextElement(startOrder, reverse, group, first, closest, false, (FocusFlags & EFF_CAN_FOCUS_DISABLED) != 0);

	if (closest)
		return closest; // we found an element
	else if (first)
		return first; // go to the end or the start
	else if (group)
		return this; // no group found? root group
	else
		return 0;
}

void CGUIEnvironment::setFocusBehavior(u32 flags)
{
	FocusFlags = flags;
}

u32 CGUIEnvironment::getFocusBehavior() const
{
	return FocusFlags;
}

//! creates an GUI Environment
IGUIEnvironment *createGUIEnvironment(io::IFileSystem *fs,
		video::IVideoDriver *Driver,
		IOSOperator *op)
{
	return new CGUIEnvironment(fs, Driver, op);
}

} // end namespace gui
