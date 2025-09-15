// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2020 DS

#include "guiScrollContainer.h"
#include "gui/IGUIEnvironment.h"
#include <Core/Events.h>

GUIScrollContainer::GUIScrollContainer(gui::IGUIEnvironment *env,
		gui::IGUIElement *parent, s32 id, const recti &rectangle,
		const std::string &orientation, f32 scrollfactor) :
		gui::IGUIElement(EGUIET_ELEMENT, env, parent, id, rectangle),
		m_scrollbar(nullptr), m_scrollfactor(scrollfactor)
{
	if (orientation == "vertical")
		m_orientation = VERTICAL;
	else if (orientation == "horizontal")
		m_orientation = HORIZONTAL;
	else
		m_orientation = UNDEFINED;
}

bool GUIScrollContainer::OnEvent(const core::Event &event)
{
	if (event.Type == EET_MOUSE_INPUT_EVENT &&
            event.MouseInput.Type == EMIE_MOUSE_WHEEL &&
			!event.MouseInput.isLeftPressed() && m_scrollbar) {
		Environment->setFocus(m_scrollbar);
		bool retval = m_scrollbar->OnEvent(event);

		// a hacky fix for updating the hovering and co.
		IGUIElement *hovered_elem = getElementFromPoint(v2i(
				event.MouseInput.X, event.MouseInput.Y));
		core::Event mov_event = event;
        mov_event.MouseInput.Type = EMIE_MOUSE_MOVED;
		Environment->postEventFromUser(mov_event);
		if (hovered_elem)
			hovered_elem->OnEvent(mov_event);

		return retval;
	}

	return IGUIElement::OnEvent(event);
}

void GUIScrollContainer::draw()
{
	if (isVisible()) {
		for (auto child : Children)
			if (child->isNotClipped() ||
					AbsoluteClippingRect.isRectCollided(
							child->getAbsolutePosition()))
				child->draw();
	}
}

void GUIScrollContainer::setScrollBar(GUIScrollBar *scrollbar)
{
	m_scrollbar = scrollbar;

	if (m_scrollbar && m_content_padding_px.has_value() && m_scrollfactor != 0.0f) {
		// Set the scrollbar max value based on the content size.

		// Get content size based on elements
		recti size;
		for (gui::IGUIElement *e : Children) {
			recti abs_rect = e->getAbsolutePosition();
			size.addInternalPoint(abs_rect.LRC);
		}

		s32 visible_content_px = (
			m_orientation == VERTICAL
				? AbsoluteClippingRect.getHeight()
				: AbsoluteClippingRect.getWidth()
		);

		s32 total_content_px = *m_content_padding_px + (
			m_orientation == VERTICAL
				? (size.LRC.Y - AbsoluteClippingRect.ULC.Y)
				: (size.LRC.X - AbsoluteClippingRect.ULC.X)
		);

		s32 hidden_content_px = std::max<s32>(0, total_content_px - visible_content_px);
		m_scrollbar->setMin(0);
		m_scrollbar->setMax(std::ceil(hidden_content_px / std::fabs(m_scrollfactor)));

		// Note: generally, the scrollbar has the same size as the scroll container.
		// However, in case it isn't, proportional adjustments are needed.
		s32 scrollbar_px = (
			m_scrollbar->isHorizontal()
				? m_scrollbar->getRelativePosition().getWidth()
				: m_scrollbar->getRelativePosition().getHeight()
		);

		m_scrollbar->setPageSize((total_content_px * scrollbar_px) / visible_content_px);
	}

	updateScrolling();
}

void GUIScrollContainer::updateScrolling()
{
	s32 pos = m_scrollbar->getPos();
	recti rect = getRelativePosition();

	if (m_orientation == VERTICAL)
		rect.ULC.Y = pos * m_scrollfactor;
	else if (m_orientation == HORIZONTAL)
		rect.ULC.X = pos * m_scrollfactor;

	setRelativePosition(rect);
}
