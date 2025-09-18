/*
Part of Minetest
Copyright (C) 2023-24 rubenwardy

Permission to use, copy, modify, and distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include "guiOpenURL.h"
#include "gui/IGUIEnvironment.h"
#include "guiButton.h"
#include "guiEditBoxWithScrollbar.h"
#include "IGUIEditBox.h"
#include <Render/TTFont.h>
#include "client/render/rendersystem.h"
#include "porting.h"
#include "gettext.h"
#include "util/colorize.h"

namespace {
	constexpr int ID_url = 256;
	constexpr int ID_open = 259;
	constexpr int ID_cancel = 261;
}

GUIOpenURLMenu::GUIOpenURLMenu(gui::IGUIEnvironment* env,
        gui::IGUIElement* parent, s32 id,
        IMenuManager *menumgr, const std::string &url
):
	GUIModalMenu(env, parent, id, menumgr),
    url(url),
    openURLBox(std::make_unique<UISprite>(nullptr, env->getRenderSystem()->getRenderer(),
        env->getResourceCache(), std::vector<UIPrimitiveType>{UIPrimitiveType::RECTANGLE}, true))
{
}

static std::string maybe_colorize_url(const std::string &url)
{
	// Forbid escape codes in URL
	if (url.find('\x1b') != std::string::npos) {
		throw std::runtime_error("URL contains escape codes");
	}

#ifdef HAVE_COLORIZE_URL
	return colorize_url(url);
#else
	return url;
#endif
}

void GUIOpenURLMenu::regenerateGui(v2u screensize)
{
	/*
		Remove stuff
	*/
	removeAllChildren();

	/*
		Calculate new sizes and positions
	*/
	ScalingInfo info = getScalingInfo(screensize, v2u(580, 250));
	const float s = info.scale;
	DesiredRect = info.rect;
	recalculateAbsolutePosition(false);

	v2i size = DesiredRect.getSize();
	v2i topleft_client(40 * s, 0);

	/*
		Get URL text
	*/
	bool ok = true;
	std::string text;
	try {
		text = maybe_colorize_url(url);
	} catch (const std::exception &e) {
		text = std::string(e.what()) + " (url = " + url + ")";
		ok = false;
	}

	/*
		Add stuff
	*/
	s32 ypos = 40 * s;

	{
		recti rect(0, 0, 500 * s, 20 * s);
		rect += topleft_client + v2i(20 * s, ypos);

		std::wstring title = ok
				? wstrgettext("Open URL?")
				: wstrgettext("Unable to open URL");
		gui::StaticText::add(Environment, title, rect,
				false, true, this, -1);
	}

	ypos += 50 * s;

	{
		recti rect(0, 0, 440 * s, 60 * s);

        auto font_mgr = Environment->getRenderSystem()->getFontManager();
        auto font = font_mgr->getFontOrCreate(
            ok ? render::FontMode::MONO : render::FontMode::GRAY, render::FontStyle::NORMAL,
            font_mgr->getDefaultFontSize(ok ? render::FontMode::MONO : render::FontMode::GRAY));
		int scrollbar_width = Environment->getSkin()->getSize(EGDS_SCROLLBAR_SIZE);
        int max_cols = (rect.getWidth() - scrollbar_width - 10) / font->getTextWidth(L"x");

		text = wrap_rows(text, max_cols, true);

		rect += topleft_client + v2i(20 * s, ypos);
		IGUIEditBox *e = new GUIEditBoxWithScrollBar(utf8_to_wide(text).c_str(), true, Environment,
                this, ID_url, rect, false, true);
		e->setMultiLine(true);
		e->setWordWrap(true);
		e->setTextAlignment(EGUIA_UPPERLEFT, EGUIA_UPPERLEFT);
		e->setDrawBorder(true);
		e->setDrawBackground(true);
		e->setOverrideFont(font);
		e->drop();
	}

	ypos += 80 * s;
	if (ok) {
		recti rect(0, 0, 100 * s, 40 * s);
		rect = rect + v2i(size.X / 2 - 150 * s, ypos);
        GUIButton::addButton(Environment, rect, this, ID_open,
				wstrgettext("Open").c_str());
	}
	{
		recti rect(0, 0, 100 * s, 40 * s);
		rect = rect + v2i(size.X / 2 + 50 * s, ypos);
        GUIButton::addButton(Environment, rect, this, ID_cancel,
				wstrgettext("Cancel").c_str());
	}
}

void GUIOpenURLMenu::drawMenu()
{
    GUISkin *skin = Environment->getSkin();
	if (!skin)
		return;

    img::color8 bgcolor(img::PF_RGBA8, 0, 0, 0, 140);
    openURLBox->getShape()->updateRectangle(0, toRectf(AbsoluteRect), {bgcolor});
    openURLBox->updateMesh();
    openURLBox->setClipRect(AbsoluteClippingRect);
    openURLBox->draw();

	gui::IGUIElement::draw();
#ifdef __ANDROID__
	getAndroidUIInput();
#endif
}

bool GUIOpenURLMenu::OnEvent(const core::Event &event)
{
	if (event.Type == EET_KEY_INPUT_EVENT) {
		if (event.KeyInput.Key == core::KEY_ESCAPE && event.KeyInput.PressedDown) {
			quitMenu();
			return true;
		}
		if (event.KeyInput.Key == core::KEY_RETURN && event.KeyInput.PressedDown) {
			porting::open_url(url);
			quitMenu();
			return true;
		}
	}

	if (event.Type == EET_GUI_EVENT) {
		if (event.GUI.Type == EGET_ELEMENT_FOCUS_LOST &&
				isVisible()) {
            if (!canTakeFocus(event.GUI.Element.value())) {
				infostream << "GUIOpenURLMenu: Not allowing focus change."
					<< std::endl;
				// Returning true disables focus change
				return true;
			}
		}

		if (event.GUI.Type == EGET_BUTTON_CLICKED) {
            switch (event.GUI.Caller.value()) {
			case ID_open:
				porting::open_url(url);
				quitMenu();
				return true;
			case ID_cancel:
				quitMenu();
				return true;
			}
		}
	}

	return Parent != nullptr && Parent->OnEvent(event);
}
