// Copyright (C) 2002-2012 Nikolaus Gebhardt
// Modified by Mustapha T.
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "guiEditBoxWithScrollbar.h"

#include "GUISkin.h"
#include "IGUIEnvironment.h"
#include "client/render/rendersystem.h"
#include <Render/TTFont.h>
#include "porting.h"
#include <Core/Keycodes.h>
#include "client/ui/sprite.h"
#include "util/enriched_string.h"
#include "client/ui/extra_images.h"

/*
todo:
optional scrollbars [done]
ctrl+left/right to select word
double click/ctrl click: word select + drag to select whole words, triple click to select line
optional? dragging selected text
numerical
*/

//! constructor
GUIEditBoxWithScrollBar::GUIEditBoxWithScrollBar(const wchar_t* text, bool border,
	IGUIEnvironment* environment, IGUIElement* parent, s32 id,
    const recti& rectangle,
	bool writable, bool has_vscrollbar)
	: GUIEditBox(environment, parent, id, rectangle, border, writable),
    m_background(true), m_bg_color_used(false),
    m_editbox_bank(std::make_unique<UISpriteBank>(environment->getRenderSystem(), environment->getResourceCache()))
{

	Text = text;

	if (Environment)
        m_operator = Environment->getRenderSystem()->getWindow()->getClipboard();

	// this element can be tabbed to
	setTabStop(true);
	setTabOrder(-1);

	if (has_vscrollbar) {
		createVScrollBar();
	}

	calculateFrameRect();
	breakText();

	calculateScrollPos();
	setWritable(writable);
}

//! Sets whether to draw the background
void GUIEditBoxWithScrollBar::setDrawBackground(bool draw)
{
	m_background = draw;
}


void GUIEditBoxWithScrollBar::updateAbsolutePosition()
{
	recti old_absolute_rect(AbsoluteRect);
	IGUIElement::updateAbsolutePosition();
	if (old_absolute_rect != AbsoluteRect) {
		calculateFrameRect();
		breakText();
		calculateScrollPos();
	}
}

void GUIEditBoxWithScrollBar::updateMesh()
{
    const bool focus = Environment->hasFocus(this);

    GUISkin* skin = Environment->getSkin();
    if (!skin)
        return;

    img::color8 default_bg_color;
    img::color8 bg_color;

    EGUI_DEFAULT_COLOR bgCol = EGDC_GRAY_EDITABLE;
    if (isEnabled())
        bgCol = focus ? EGDC_FOCUSED_EDITABLE : EGDC_EDITABLE;

    m_editbox_bank->clear();

    if ((!m_border && m_background) || m_border) {
        UIRects *editBoxRects = m_editbox_bank->addSprite({}, &AbsoluteClippingRect);

        if (!m_border && m_background)
            editBoxRects->addRect(toRectT<f32>(AbsoluteRect), {skin->getColor(bgCol)});

        // draw the border
        if (m_border)
            skin->add3DSunkenPane(editBoxRects, skin->getColor(bgCol), false, m_background, toRectT<f32>(AbsoluteRect));

        editBoxRects->rebuildMesh();
    }

    calculateFrameRect();

    recti local_clip_rect = m_frame_rect;
    local_clip_rect.clipAgainst(AbsoluteClippingRect);

    // draw the text

    render::TTFont* font = getActiveFont();

    s32 cursor_line = 0;
    s32 charcursorpos = 0;

    if (font) {
        if (m_last_break_font != font) {
            breakText();
        }

        // calculate cursor pos

        std::wstring *txt_line = &Text;
        s32 start_pos = 0;

        std::wstring s, s2;

        // get mark position
        const bool ml = (!m_passwordbox && (m_word_wrap || m_multiline));
        const s32 realmbgn = m_mark_begin < m_mark_end ? m_mark_begin : m_mark_end;
        const s32 realmend = m_mark_begin < m_mark_end ? m_mark_end : m_mark_begin;
        const s32 hline_start = ml ? getLineFromPos(realmbgn) : 0;
        const s32 hline_count = ml ? getLineFromPos(realmend) - hline_start + 1 : 1;
        const s32 line_count = ml ? m_broken_text.size() : 1;

        // Save the override color information.
        // Then, alter it if the edit box is disabled.
        const bool prevOver = m_override_color_enabled;
        const img::color8 prevColor = m_override_color;

        if (Text.size()) {
            if (!isEnabled() && !m_override_color_enabled) {
                m_override_color_enabled = true;
                m_override_color = skin->getColor(EGDC_GRAY_TEXT);
            }

            for (s32 i = 0; i < line_count; ++i) {
                setTextRect(i);

                // clipping test - don't draw anything outside the visible area
                recti c = local_clip_rect;
                c.clipAgainst(m_current_text_rect);
                if (!c.isValid())
                    continue;

                // get current line
                if (m_passwordbox) {
                    if (m_broken_text.size() != 1) {
                        m_broken_text.clear();
                        m_broken_text.emplace_back();
                    }
                    if (m_broken_text[0].size() != Text.size()){
                        m_broken_text[0] = Text;
                        for (u32 q = 0; q < Text.size(); ++q)
                        {
                            m_broken_text[0][q] = m_passwordchar;
                        }
                    }
                    txt_line = &m_broken_text[0];
                    start_pos = 0;
                } else {
                    txt_line = ml ? &m_broken_text[i] : &Text;
                    start_pos = ml ? m_broken_text_positions[i] : 0;
                }


                // draw normal text
                m_editbox_bank->addTextSprite(*txt_line, toRectT<f32>(m_current_text_rect),
                    m_override_color_enabled ? m_override_color : skin->getColor(EGDC_BUTTON_TEXT), &local_clip_rect);

                // draw mark and marked text
                if (focus && m_mark_begin != m_mark_end && i >= hline_start && i < hline_start + hline_count) {

                    s32 mbegin = 0, mend = 0;
                    s32 lineStartPos = 0, lineEndPos = txt_line->size();

                    if (i == hline_start) {
                        // highlight start is on this line
                        s = txt_line->substr(0, realmbgn - start_pos);
                        mbegin = font->getTextWidth(s);

                        // deal with kerning
                        mbegin += font->getKerningSizeForTwoChars(
                            (*txt_line)[realmbgn - start_pos],
                            realmbgn - start_pos > 0 ? (*txt_line)[realmbgn - start_pos - 1] : 0);

                        lineStartPos = realmbgn - start_pos;
                    }
                    if (i == hline_start + hline_count - 1) {
                        // highlight end is on this line
                        s2 = txt_line->substr(0, realmend - start_pos);
                        mend = font->getTextWidth(s2);
                        lineEndPos = (s32)s2.size();
                    } else {
                        mend = font->getTextWidth(*txt_line);
                    }


                    m_current_text_rect.ULC.X += mbegin;
                    m_current_text_rect.LRC.X = m_current_text_rect.ULC.X + mend - mbegin;


                    // draw mark
                    m_editbox_bank->addSprite({{toRectT<f32>(m_current_text_rect), {skin->getColor(EGDC_HIGH_LIGHT)}}}, &local_clip_rect);

                    // draw marked text
                    s = txt_line->substr(lineStartPos, lineEndPos - lineStartPos);

                    if (s.size())
                        m_editbox_bank->addTextSprite(s, toRectT<f32>(m_current_text_rect),
                            m_override_color_enabled ? m_override_color : skin->getColor(EGDC_HIGH_LIGHT_TEXT),  &local_clip_rect);
                }
            }

            // Return the override color information to its previous settings.
            m_override_color_enabled = prevOver;
            m_override_color = prevColor;
        }

        // draw cursor
        if (isEnabled()) {
            if (m_word_wrap || m_multiline) {
                cursor_line = getLineFromPos(m_cursor_pos);
                txt_line = &m_broken_text[cursor_line];
                start_pos = m_broken_text_positions[cursor_line];
            }
            s = txt_line->substr(0, m_cursor_pos - start_pos);
            charcursorpos = font->getTextWidth(s);/* +
                font->getKerningSizeForTwoChars(L'_',
                    m_cursor_pos - start_pos > 0 ? (*txt_line)[m_cursor_pos - start_pos - 1] : 0);*/

            if (focus && (porting::getTimeMs() - m_blink_start_time) % 700 < 350) {
                setTextRect(cursor_line);
                m_current_text_rect.ULC.X += charcursorpos;

                m_editbox_bank->addTextSprite(L"_", toRectT<f32>(m_current_text_rect),
                    m_override_color_enabled ? m_override_color : skin->getColor(EGDC_BUTTON_TEXT), &local_clip_rect);
            }
        }
    }
}

//! draws the element and its children
void GUIEditBoxWithScrollBar::draw()
{
    if (!IsVisible)
		return;

    updateMesh();
    m_editbox_bank->drawBank();

	// draw children
    IGUIElement::draw();
}


s32 GUIEditBoxWithScrollBar::getCursorPos(s32 x, s32 y)
{
	render::TTFont* font = getActiveFont();

	const u32 line_count = (m_word_wrap || m_multiline) ? m_broken_text.size() : 1;

	std::wstring *txt_line = 0;
	s32 start_pos = 0;
	x += 3;

	for (u32 i = 0; i < line_count; ++i) {
		setTextRect(i);
		if (i == 0 && y < m_current_text_rect.ULC.Y)
			y = m_current_text_rect.ULC.Y;
		if (i == line_count - 1 && y > m_current_text_rect.LRC.Y)
			y = m_current_text_rect.LRC.Y;

		// is it inside this region?
		if (y >= m_current_text_rect.ULC.Y && y <= m_current_text_rect.LRC.Y) {
			// we've found the clicked line
			txt_line = (m_word_wrap || m_multiline) ? &m_broken_text[i] : &Text;
			start_pos = (m_word_wrap || m_multiline) ? m_broken_text_positions[i] : 0;
			break;
		}
	}

	if (x < m_current_text_rect.ULC.X)
		x = m_current_text_rect.ULC.X;

	if (!txt_line)
		return 0;

    s32 idx = font->getCharFromPos(txt_line->c_str(), x - m_current_text_rect.ULC.X);

	// click was on or left of the line
	if (idx != -1)
		return idx + start_pos;

	// click was off the right edge of the line, go to end.
	return txt_line->size() + start_pos;
}


//! Breaks the single text line.
void GUIEditBoxWithScrollBar::breakText()
{
	if ((!m_word_wrap && !m_multiline))
		return;

	m_broken_text.clear(); // need to reallocate :/
	m_broken_text_positions.clear();

	render::TTFont* font = getActiveFont();
	if (!font)
		return;

	m_last_break_font = font;

	std::wstring line;
	std::wstring word;
	std::wstring whitespace;
	s32 last_line_start = 0;
	s32 size = Text.size();
	s32 length = 0;
	s32 el_width = RelativeRect.getWidth() - m_scrollbar_width - 10;
	wchar_t c;

	for (s32 i = 0; i < size; ++i) {
		c = Text[i];
		bool line_break = false;

		if (c == L'\r') { // Mac or Windows breaks

			line_break = true;
			c = 0;
			if (Text[i + 1] == L'\n') { // Windows breaks
				// TODO: I (Michael) think that we shouldn't change the text given by the user for whatever reason.
				// Instead rework the cursor positioning to be able to handle this (but not in stable release
				// branch as users might already expect this behavior).
				Text.erase(i + 1);
				--size;
				if (m_cursor_pos > i)
					--m_cursor_pos;
			}
		} else if (c == L'\n') { // Unix breaks
			line_break = true;
			c = 0;
		}

		// don't break if we're not a multi-line edit box
		if (!m_multiline)
			line_break = false;

		if (c == L' ' || c == 0 || i == (size - 1)) {
			// here comes the next whitespace, look if
			// we can break the last word to the next line
			// We also break whitespace, otherwise cursor would vanish beside the right border.
            s32 whitelgth = font->getTextWidth(whitespace.c_str());
            s32 worldlgth = font->getTextWidth(word.c_str());

			if (m_word_wrap && length + worldlgth + whitelgth > el_width && line.size() > 0) {
				// break to next line
				length = worldlgth;
				m_broken_text.push_back(line);
				m_broken_text_positions.push_back(last_line_start);
				last_line_start = i - (s32)word.size();
				line = word;
			} else {
				// add word to line
				line += whitespace;
				line += word;
				length += whitelgth + worldlgth;
			}

			word = L"";
			whitespace = L"";


			if (c)
				whitespace += c;

			// compute line break
			if (line_break) {
				line += whitespace;
				line += word;
				m_broken_text.push_back(line);
				m_broken_text_positions.push_back(last_line_start);
				last_line_start = i + 1;
				line = L"";
				word = L"";
				whitespace = L"";
				length = 0;
			}
		} else {
			// yippee this is a word..
			word += c;
		}
	}

	line += whitespace;
	line += word;
	m_broken_text.push_back(line);
	m_broken_text_positions.push_back(last_line_start);
}

// TODO: that function does interpret VAlign according to line-index (indexed
// line is placed on top-center-bottom) but HAlign according to line-width
// (pixels) and not by row.
// Intuitively I suppose HAlign handling is better as VScrollPos should handle
// the line-scrolling.
// But please no one change this without also rewriting (and this time
// testing!!!) autoscrolling (I noticed this when fixing the old autoscrolling).
void GUIEditBoxWithScrollBar::setTextRect(s32 line)
{
	if (line < 0)
		return;

	render::TTFont* font = getActiveFont();
	if (!font)
		return;

	v2u d;

	// get text dimension
	const u32 line_count = (m_word_wrap || m_multiline) ? m_broken_text.size() : 1;
	if (m_word_wrap || m_multiline) {
        d = font->getTextSize(m_broken_text[line].c_str());
	} else {
        d = font->getTextSize(Text.c_str());
		d.Y = AbsoluteRect.getHeight();
	}
    d.Y += font->getKerningSizeForTwoChars(L'A', L'B');

	// justification
	switch (m_halign) {
	case EGUIA_CENTER:
		// align to h center
		m_current_text_rect.ULC.X = (m_frame_rect.getWidth() / 2) - (d.X / 2);
		m_current_text_rect.LRC.X = (m_frame_rect.getWidth() / 2) + (d.X / 2);
		break;
	case EGUIA_LOWERRIGHT:
		// align to right edge
		m_current_text_rect.ULC.X = m_frame_rect.getWidth() - d.X;
		m_current_text_rect.LRC.X = m_frame_rect.getWidth();
		break;
	default:
		// align to left edge
		m_current_text_rect.ULC.X = 0;
		m_current_text_rect.LRC.X = d.X;

	}

	switch (m_valign) {
	case EGUIA_CENTER:
		// align to v center
		m_current_text_rect.ULC.Y =
			(m_frame_rect.getHeight() / 2) - (line_count*d.Y) / 2 + d.Y*line;
		break;
	case EGUIA_LOWERRIGHT:
		// align to bottom edge
		m_current_text_rect.ULC.Y =
			m_frame_rect.getHeight() - line_count*d.Y + d.Y*line;
		break;
	default:
		// align to top edge
		m_current_text_rect.ULC.Y = d.Y*line;
		break;
	}

	m_current_text_rect.ULC.X -= m_hscroll_pos;
	m_current_text_rect.LRC.X -= m_hscroll_pos;
	m_current_text_rect.ULC.Y -= m_vscroll_pos;
	m_current_text_rect.LRC.Y = m_current_text_rect.ULC.Y + d.Y;

	m_current_text_rect += m_frame_rect.ULC;
}

// calculate autoscroll
void GUIEditBoxWithScrollBar::calculateScrollPos()
{
	if (!m_autoscroll)
		return;

    GUISkin* skin = Environment->getSkin();
	if (!skin)
		return;
	render::TTFont* font = m_override_font ? m_override_font : skin->getFont();
	if (!font)
		return;

	s32 curs_line = getLineFromPos(m_cursor_pos);
	if (curs_line < 0)
		return;
	setTextRect(curs_line);
	const bool has_broken_text = m_multiline || m_word_wrap;

	// Check horizonal scrolling
	// NOTE: Calculations different to vertical scrolling because setTextRect interprets VAlign relative to line but HAlign not relative to row
	{
		// get cursor position
		render::TTFont* font = getActiveFont();
		if (!font)
			return;

		// get cursor area
        u32 cursor_width = font->getTextWidth(L"_");
		std::wstring *txt_line = has_broken_text ? &m_broken_text[curs_line] : &Text;
		s32 cpos = has_broken_text ? m_cursor_pos - m_broken_text_positions[curs_line] : m_cursor_pos;	// column
        s32 cstart = font->getTextWidth(txt_line->substr(0, cpos).c_str());		// pixels from text-start
		s32 cend = cstart + cursor_width;
        s32 txt_width = font->getTextWidth(txt_line->c_str());

		if (txt_width < m_frame_rect.getWidth()) {
			// TODO: Needs a clean left and right gap removal depending on HAlign, similar to vertical scrolling tests for top/bottom.
			// This check just fixes the case where it was most noticeable (text smaller than clipping area).

			m_hscroll_pos = 0;
			setTextRect(curs_line);
		}

		if (m_current_text_rect.ULC.X + cstart < m_frame_rect.ULC.X) {
			// cursor to the left of the clipping area
			m_hscroll_pos -= m_frame_rect.ULC.X - (m_current_text_rect.ULC.X + cstart);
			setTextRect(curs_line);

			// TODO: should show more characters to the left when we're scrolling left
			//	and the cursor reaches the border.
		} else if (m_current_text_rect.ULC.X + cend > m_frame_rect.LRC.X)	{
			// cursor to the right of the clipping area
			m_hscroll_pos += (m_current_text_rect.ULC.X + cend) - m_frame_rect.LRC.X;
			setTextRect(curs_line);
		}
	}

	// calculate vertical scrolling
	if (has_broken_text) {
        u32 line_height = font->getTextHeight(L"A") + font->getKerningSizeForTwoChars(L'A', L'B');
		// only up to 1 line fits?
        if (line_height >= (u32)m_frame_rect.getHeight()) {
			m_vscroll_pos = 0;
			setTextRect(curs_line);
			s32 unscrolledPos = m_current_text_rect.ULC.Y;
			s32 pivot = m_frame_rect.ULC.Y;
			switch (m_valign) {
			case EGUIA_CENTER:
				pivot += m_frame_rect.getHeight() / 2;
				unscrolledPos += line_height / 2;
				break;
			case EGUIA_LOWERRIGHT:
				pivot += m_frame_rect.getHeight();
				unscrolledPos += line_height;
				break;
			default:
				break;
			}
			m_vscroll_pos = unscrolledPos - pivot;
			setTextRect(curs_line);
		} else {
			// First 2 checks are necessary when people delete lines
			setTextRect(0);
			if (m_current_text_rect.ULC.Y > m_frame_rect.ULC.Y && m_valign != EGUIA_LOWERRIGHT) {
				// first line is leaving a gap on top
				m_vscroll_pos = 0;
			} else if (m_valign != EGUIA_UPPERLEFT) {
				u32 lastLine = m_broken_text_positions.empty() ? 0 : m_broken_text_positions.size() - 1;
				setTextRect(lastLine);
				if (m_current_text_rect.LRC.Y < m_frame_rect.LRC.Y)
				{
					// last line is leaving a gap on bottom
					m_vscroll_pos -= m_frame_rect.LRC.Y - m_current_text_rect.LRC.Y;
				}
			}

			setTextRect(curs_line);
			if (m_current_text_rect.ULC.Y < m_frame_rect.ULC.Y) {
				// text above valid area
				m_vscroll_pos -= m_frame_rect.ULC.Y - m_current_text_rect.ULC.Y;
				setTextRect(curs_line);
			} else if (m_current_text_rect.LRC.Y > m_frame_rect.LRC.Y){
				// text below valid area
				m_vscroll_pos += m_current_text_rect.LRC.Y - m_frame_rect.LRC.Y;
				setTextRect(curs_line);
			}
		}
	}

	if (m_vscrollbar) {
		m_vscrollbar->setPos(m_vscroll_pos);
	}
}

void GUIEditBoxWithScrollBar::calculateFrameRect()
{
	m_frame_rect = AbsoluteRect;

    GUISkin *skin = 0;
	if (Environment)
		skin = Environment->getSkin();
	if (m_border && skin) {
		m_frame_rect.ULC.X += skin->getSize(EGDS_TEXT_DISTANCE_X) + 1;
		m_frame_rect.ULC.Y += skin->getSize(EGDS_TEXT_DISTANCE_Y) + 1;
		m_frame_rect.LRC.X -= skin->getSize(EGDS_TEXT_DISTANCE_X) + 1;
		m_frame_rect.LRC.Y -= skin->getSize(EGDS_TEXT_DISTANCE_Y) + 1;
	}

	updateVScrollBar();
}

//! create a vertical scroll bar
void GUIEditBoxWithScrollBar::createVScrollBar()
{
    GUISkin *skin = 0;
	if (Environment)
		skin = Environment->getSkin();

	s32 fontHeight = 1;

	if (m_override_font) {
        fontHeight = m_override_font->getFontHeight();
	} else {
		render::TTFont *font;
		if (skin && (font = skin->getFont())) {
            fontHeight = font->getFontHeight();
		}
	}

	m_scrollbar_width = skin ? skin->getSize(EGDS_SCROLLBAR_SIZE) : 16;

    recti scrollbarrect = m_frame_rect;
	scrollbarrect.ULC.X += m_frame_rect.getWidth() - m_scrollbar_width;
	m_vscrollbar = new GUIScrollBar(Environment, getParent(), -1,
            scrollbarrect, false, true);

	m_vscrollbar->setVisible(false);
	m_vscrollbar->setSmallStep(3 * fontHeight);
	m_vscrollbar->setLargeStep(10 * fontHeight);
}



//! Change the background color
void GUIEditBoxWithScrollBar::setBackgroundColor(const img::color8 &bg_color)
{
	m_bg_color = bg_color;
	m_bg_color_used = true;
}

bool GUIEditBoxWithScrollBar::isDrawBackgroundEnabled() const { return false; }
bool GUIEditBoxWithScrollBar::isDrawBorderEnabled() const { return false; }
void GUIEditBoxWithScrollBar::setCursorChar(const wchar_t cursorChar) { }
wchar_t GUIEditBoxWithScrollBar::getCursorChar() const { return '|'; }
void GUIEditBoxWithScrollBar::setCursorBlinkTime(u32 timeMs) { }
u32 GUIEditBoxWithScrollBar::getCursorBlinkTime() const { return 500; }
