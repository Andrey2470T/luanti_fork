// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#pragma once

#include "IGUIEnvironment.h"
#include "IGUIElement.h"

namespace gui
{

class CGUIEnvironment : public IGUIEnvironment, public IGUIElement
{
public:
	//! constructor
    CGUIEnvironment(RenderSystem *rndsys, v2u wnd_size, ResourceCache *rescache);

	//! destructor
	virtual ~CGUIEnvironment();

    RenderSystem *getRenderSystem() const override
    {
        return RndSys;
    }
    ResourceCache *getResourceCache() const override
    {
        return ResCache;
    }
	//! draws all gui elements
	void drawAll(bool useScreenSize) override;

	//! posts an input event to the environment
	bool postEventFromUser(const core::Event &event) override;

	//! This sets a new event receiver for gui events. Usually you do not have to
	//! use this method, it is used by the internal engine.
    void setUserEventReceiver(core::IEventReceiver *evr) override;

	//! removes all elements from the environment
	void clear() override;

	//! called if an event happened.
	bool OnEvent(const core::Event &event) override;

	//! returns the current gui skin
    GUISkin *getSkin() const override;

	//! Sets a new GUI Skin
    void setSkin(GUISkin *skin) override;

	//! Creates a new GUI Skin.
	/** \return Returns a pointer to the created skin.
    If you no longer need the skin, you should call GUISkin::drop().
	See IReferenceCounted::drop() for more information. */
    GUISkin *createSkin() override;

	//! Creates the image list from the given texture.
	virtual IGUIImageList *createImageList(img::Image *texture,
            v2i imageSize, bool useAlphaChannel) override;

	//! returns the font
    render::TTFont *getFont(const std::string &filename) override;

	//! add an externally loaded font
    render::TTFont *addFont(const std::string &name, render::TTFont *font) override;

	//! remove loaded font
	void removeFont(render::TTFont *font) override;

	//! returns default font
	render::TTFont *getBuiltInFont() const override;

	//! returns the sprite bank
    IGUISpriteBank *getSpriteBank(const std::string &filename) override;

	//! returns the sprite bank
    IGUISpriteBank *addEmptySpriteBank(const std::string &name) override;

	//! adds an button. The returned pointer must not be dropped.
	IGUIButton *addButton(const recti &rectangle, IGUIElement *parent = 0, s32 id = -1, const wchar_t *text = 0, const wchar_t *tooltiptext = 0) override;

	//! adds a scrollbar. The returned pointer must not be dropped.
	virtual IGUIScrollBar *addScrollBar(bool horizontal, const recti &rectangle,
			IGUIElement *parent = 0, s32 id = -1) override;

	//! Adds an image element.
	virtual IGUIImage *addImage(img::Image *image, v2i pos,
			bool useAlphaChannel = true, IGUIElement *parent = 0, s32 id = -1, const wchar_t *text = 0) override;

	//! adds an image. The returned pointer must not be dropped.
	virtual IGUIImage *addImage(const recti &rectangle,
			IGUIElement *parent = 0, s32 id = -1, const wchar_t *text = 0, bool useAlphaChannel = true) override;

	//! adds a checkbox
	virtual IGUICheckBox *addCheckBox(bool checked, const recti &rectangle,
			IGUIElement *parent = 0, s32 id = -1, const wchar_t *text = 0) override;

	//! adds a list box
	virtual IGUIListBox *addListBox(const recti &rectangle,
			IGUIElement *parent = 0, s32 id = -1, bool drawBackground = false) override;

	//! Adds a file open dialog.
	virtual IGUIFileOpenDialog *addFileOpenDialog(const wchar_t *title = 0,
			bool modal = true, IGUIElement *parent = 0, s32 id = -1,
            bool restoreCWD = false, std::string *startDir = 0) override;

	//! adds a static text. The returned pointer must not be dropped.
	virtual IGUIStaticText *addStaticText(const wchar_t *text, const recti &rectangle,
			bool border = false, bool wordWrap = true, IGUIElement *parent = 0, s32 id = -1, bool drawBackground = false) override;

	//! Adds an edit box. The returned pointer must not be dropped.
	virtual IGUIEditBox *addEditBox(const wchar_t *text, const recti &rectangle,
			bool border = false, IGUIElement *parent = 0, s32 id = -1) override;

	//! Adds a tab control to the environment.
	virtual IGUITabControl *addTabControl(const recti &rectangle,
			IGUIElement *parent = 0, bool fillbackground = false, bool border = true, s32 id = -1) override;

	//! Adds tab to the environment.
	virtual IGUITab *addTab(const recti &rectangle,
			IGUIElement *parent = 0, s32 id = -1) override;

	//! Adds a combo box to the environment.
	virtual IGUIComboBox *addComboBox(const recti &rectangle,
			IGUIElement *parent = 0, s32 id = -1) override;

	//! sets the focus to an element
	bool setFocus(IGUIElement *element) override;

	//! removes the focus from an element
	bool removeFocus(IGUIElement *element) override;

	//! Returns if the element has focus
	bool hasFocus(const IGUIElement *element, bool checkSubElements = false) const override;

	//! Returns the element with the focus
	IGUIElement *getFocus() const override;

	//! Returns the element last known to be under the mouse
	IGUIElement *getHovered() const override;

	//! Returns the root gui element.
	IGUIElement *getRootGUIElement() override;

	void OnPostRender(u32 time) override;

	//! Find the next element which would be selected when pressing the tab-key
	IGUIElement *getNextElement(bool reverse = false, bool group = false) override;

	//! Set the way the gui will handle focus changes
	void setFocusBehavior(u32 flags) override;

	//! Get the way the gui does handle focus changes
	u32 getFocusBehavior() const override;

	//! Adds a IGUIElement to deletion queue.
	void addToDeletionQueue(IGUIElement *element) override;

private:
	//! clears the deletion queue
	void clearDeletionQueue();

	void updateHoveredElement(v2i mousePos);

	void loadBuiltInFont();

	struct SFont
	{
        std::string NamedPath;
		render::TTFont *Font;

		bool operator<(const SFont &other) const
		{
			return (NamedPath < other.NamedPath);
		}
	};

	struct SSpriteBank
	{
        std::string NamedPath;
		IGUISpriteBank *Bank;

		bool operator<(const SSpriteBank &other) const
		{
			return (NamedPath < other.NamedPath);
		}
	};

	struct SToolTip
	{
		IGUIStaticText *Element;
		u32 LastTime;
		u32 EnterTime;
		u32 LaunchTime;
		u32 RelaunchTime;
	};

	SToolTip ToolTip;

	std::vector<SFont> Fonts;
	std::vector<SSpriteBank> Banks;
	IGUIElement *Hovered;
	IGUIElement *HoveredNoSubelement; // subelements replaced by their parent, so you only have 'real' elements here
	IGUIElement *Focus;
	v2i LastHoveredMousePos;
    std::unique_ptr<GUISkin> CurrentSkin;
    core::IEventReceiver *UserReceiver;
	u32 FocusFlags;
	std::vector<IGUIElement *> DeletionQueue;

    static const std::string DefaultFontName;

    RenderSystem *RndSys;
    ResourceCache *ResCache;
};

} // end namespace gui
