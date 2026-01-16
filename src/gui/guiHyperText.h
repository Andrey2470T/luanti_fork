// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2019 EvicenceBKidscode / Pierre-Yves Rollo <dev@pyrollo.com>

#pragma once

#include "IGUIElement.h"
#include <list>
#include <unordered_map>
#include <Render/TTFont.h>

class Client;
class GUIScrollBar;
class FontManager;
class SpriteDrawBatch;
class UIRects;

class ParsedText
{
public:
    ParsedText(FontManager *fontmgr, const wchar_t *text);
	~ParsedText();

	enum ElementType
	{
		ELEMENT_TEXT,
		ELEMENT_SEPARATOR,
		ELEMENT_IMAGE,
		ELEMENT_ITEM
	};

	enum BackgroundType
	{
		BACKGROUND_NONE,
		BACKGROUND_COLOR
	};

	enum FloatType
	{
		FLOAT_NONE,
		FLOAT_RIGHT,
		FLOAT_LEFT
	};

	enum HalignType
	{
		HALIGN_CENTER,
		HALIGN_LEFT,
		HALIGN_RIGHT,
		HALIGN_JUSTIFY
	};

	enum ValignType
	{
		VALIGN_MIDDLE,
		VALIGN_TOP,
		VALIGN_BOTTOM
	};

	typedef std::unordered_map<std::string, std::string> StyleList;
	typedef std::unordered_map<std::string, std::string> AttrsList;

	struct Tag
	{
		std::string name;
		AttrsList attrs;
		StyleList style;
	};

	struct Element
	{
		std::list<Tag *> tags;
		ElementType type;
        std::wstring text = L"";

		v2u dim;
		v2i pos;
		s32 drawwidth;

		FloatType floating = FLOAT_NONE;

		ValignType valign;

		render::TTFont *font;

        img::color8 color;
        img::color8 hovercolor;
		bool underline;

		s32 baseline = 0;

		// img & item specific attributes
		std::string name;
		v3s16 angle{0, 0, 0};
		v3s16 rotation{0, 0, 0};

		s32 margin = 10;

        void setStyle(FontManager *font_mgr, StyleList &style);
	};

	struct Paragraph
	{
		std::vector<Element> elements;
		HalignType halign;
		s32 margin = 10;

		void setStyle(StyleList &style);
	};

	std::vector<Paragraph> m_paragraphs;

	// Element style
	s32 margin = 3;
	ValignType valign = VALIGN_TOP;
	BackgroundType background_type = BACKGROUND_NONE;
    img::color8 background_color;

	Tag m_root_tag;

    FontManager *font_mgr;

protected:
	typedef enum { ER_NONE, ER_TAG, ER_NEWLINE } EndReason;

	// Parser functions
	void enterElement(ElementType type);
	void endElement();
	void enterParagraph();
	void endParagraph(EndReason reason);
	void pushChar(wchar_t c);
	ParsedText::Tag *newTag(const std::string &name, const AttrsList &attrs);
	ParsedText::Tag *openTag(const std::string &name, const AttrsList &attrs);
	bool closeTag(const std::string &name);
	void parseGenericStyleAttr(const std::string &name, const std::string &value,
			StyleList &style);
	void parseStyles(const AttrsList &attrs, StyleList &style);
	void globalTag(const ParsedText::AttrsList &attrs);
	u32 parseTag(const wchar_t *text, u32 cursor);
	void parse(const wchar_t *text);

	std::unordered_map<std::string, StyleList> m_elementtags;
	std::unordered_map<std::string, StyleList> m_paragraphtags;

	std::vector<Tag *> m_not_root_tags;
	std::list<Tag *> m_active_tags;

	// Current values
	StyleList m_style;
	Element *m_element;
	Paragraph *m_paragraph;
	bool m_empty_paragraph;
	EndReason m_end_paragraph_reason;
};

class TextDrawer
{
public:
    TextDrawer(const wchar_t *text, Client *client, gui::IGUIEnvironment *environment);

	void place(const recti &dest_rect);
	inline s32 getHeight() { return m_height; };
	void draw(const recti &clip_rect,
			const v2i &dest_offset);
	ParsedText::Element *getElementAt(v2i pos);
	ParsedText::Tag *m_hovertag;
    bool changed = true;

protected:
	struct RectWithMargin
	{
		recti rect;
		s32 margin;
	};

	ParsedText m_text;
	Client *m_client; ///< null in the mainmenu
	gui::IGUIEnvironment *m_guienv;
	s32 m_height;
	s32 m_voffset;
	std::vector<RectWithMargin> m_floating;

    std::unique_ptr<SpriteDrawBatch> drawBatch;
};

class GUIHyperText : public gui::IGUIElement
{
public:
	//! constructor
	GUIHyperText(const wchar_t *text, gui::IGUIEnvironment *environment,
			gui::IGUIElement *parent, s32 id,
            const recti &rectangle, Client *client);

	//! destructor
	virtual ~GUIHyperText();

	//! draws the element and its children
	virtual void draw();

	v2u getTextDimension();

	bool OnEvent(const core::Event &event);

protected:
	// GUI members
	GUIScrollBar *m_vscrollbar;
	TextDrawer m_drawer;

	// Positioning
	u32 m_scrollbar_width;
	recti m_display_text_rect;
	v2i m_text_scrollpos;

	ParsedText::Element *getElementAt(s32 X, s32 Y);
	void checkHover(s32 X, s32 Y);
};
