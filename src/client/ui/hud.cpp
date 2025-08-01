// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
// Copyright (C) 2010-2013 blue42u, Jonathon Anderson <anderjon@umail.iu.edu>
// Copyright (C) 2010-2013 kwolekr, Ryan Kwolek <kwolekr@minetest.net>

#include "hud.h"
#include "settings.h"
#include "util/numeric.h"
#include "log.h"
#include "client/client.h"
#include "inventory.h"
#include "client/render/tile.h"
#include "client/player/localplayer.h"
#include "client/player/playercamera.h"
#include "porting.h"
#include "client/mesh/meshoperations.h"
#include "client/render/wieldmesh.h"
#include "minimap.h"
#include "gui/touchcontrols.h"
#include "util/enriched_string.h"
#include "client/render/rendersystem.h"
#include "glyph_atlas.h"
#include "client/media/resource.h"
#include "batcher2d.h"

static void setting_changed_callback(const std::string &name, void *data)
{
	static_cast<Hud*>(data)->readScalingSetting();
}

Hud::Hud(Client *_client, Inventory *_inventory)
    : client(_client), player(client->getEnv().getLocalPlayer()), inventory(_inventory)
{
	readScalingSetting();
	g_settings->registerChangedCallback("dpi_change_notifier", setting_changed_callback, this);
	g_settings->registerChangedCallback("display_density_factor", setting_changed_callback, this);
	g_settings->registerChangedCallback("hud_scaling", setting_changed_callback, this);

	for (auto &hbar_color : hbar_colors)
		hbar_color = video::SColor(255, 255, 255, 255);

    v3f cross_color = g_settings->getV3F("crosshair_color").value_or(v3f());
    crosshair_color = img::color8(img::PF_RGBA8,
        myround(cross_color.X), myround(cross_color.Y), myround(cross_color.Z),
        g_settings->getS32("crosshair_alpha"));

    auto crosshair_image = cache->get<img::Image>(ResourceType::IMAGE, crosshair_img);
    auto guiPool = rnd_system->getPool(false);
    crosshair = std::make_unique<UISprite>(guiPool->getAtlasByTile(crosshair_image)->getTexture(),
        rnd_system->getRenderer(), cache, guiPool->getTileRect(crosshair_image), rectf(), std::array<img::color8, 4>{}, true);
}

void Hud::readScalingSetting()
{
    hotbar_imagesize = std::floor(HOTBAR_IMAGE_SIZE *
        rnd_system->getScaleFactor() + 0.5f);
    padding = hotbar_imagesize / 12;
}

Hud::~Hud()
{
	g_settings->deregisterAllChangedCallbacks(this);
}

void Hud::drawItem(const ItemStack &item, const core::rect<s32>& rect,
		bool selected)
{
	if (selected) {
		/* draw highlighting around selected item */
		if (use_hotbar_selected_image) {
			core::rect<s32> imgrect2 = rect;
			imgrect2.ULC.X  -= (m_padding*2);
			imgrect2.ULC.Y  -= (m_padding*2);
			imgrect2.LRC.X += (m_padding*2);
			imgrect2.LRC.Y += (m_padding*2);
				video::ITexture *texture = tsrc->getTexture(hotbar_selected_image);
				core::dimension2di imgsize(texture->getOriginalSize());
			draw2DImageFilterScaled(driver, texture, imgrect2,
					core::rect<s32>(core::position2d<s32>(0,0), imgsize),
					NULL, hbar_colors, true);
		} else {
			video::SColor c_outside(255,255,0,0);
			//video::SColor c_outside(255,0,0,0);
			//video::SColor c_inside(255,192,192,192);
			s32 x1 = rect.ULC.X;
			s32 y1 = rect.ULC.Y;
			s32 x2 = rect.LRC.X;
			s32 y2 = rect.LRC.Y;
			// Black base borders
			driver->draw2DRectangle(c_outside,
				core::rect<s32>(
				v2s32(x1 - m_padding, y1 - m_padding),
				v2s32(x2 + m_padding, y1)
				), NULL);
			driver->draw2DRectangle(c_outside,
				core::rect<s32>(
				v2s32(x1 - m_padding, y2),
				v2s32(x2 + m_padding, y2 + m_padding)
				), NULL);
			driver->draw2DRectangle(c_outside,
				core::rect<s32>(
				v2s32(x1 - m_padding, y1),
					v2s32(x1, y2)
				), NULL);
			driver->draw2DRectangle(c_outside,
				core::rect<s32>(
					v2s32(x2, y1),
				v2s32(x2 + m_padding, y2)
				), NULL);
			/*// Light inside borders
			driver->draw2DRectangle(c_inside,
				core::rect<s32>(
					v2s32(x1 - padding/2, y1 - padding/2),
					v2s32(x2 + padding/2, y1)
				), NULL);
			driver->draw2DRectangle(c_inside,
				core::rect<s32>(
					v2s32(x1 - padding/2, y2),
					v2s32(x2 + padding/2, y2 + padding/2)
				), NULL);
			driver->draw2DRectangle(c_inside,
				core::rect<s32>(
					v2s32(x1 - padding/2, y1),
					v2s32(x1, y2)
				), NULL);
			driver->draw2DRectangle(c_inside,
				core::rect<s32>(
					v2s32(x2, y1),
					v2s32(x2 + padding/2, y2)
				), NULL);
			*/
		}
	}

	video::SColor bgcolor2(128, 0, 0, 0);
	if (!use_hotbar_image)
		driver->draw2DRectangle(bgcolor2, rect, NULL);
	drawItemStack(driver, g_fontengine->getFont(), item, rect, NULL,
		client, selected ? IT_ROT_SELECTED : IT_ROT_NONE);
}

// NOTE: selectitem = 0 -> no selected; selectitem is 1-based
// mainlist can be NULL, but draw the frame anyway.
void Hud::drawItems(v2s32 screen_pos, v2s32 screen_offset, s32 itemcount, v2f alignment,
		s32 inv_offset, InventoryList *mainlist, u16 selectitem, u16 direction,
		bool is_hotbar)
{
	s32 height  = m_hotbar_imagesize + m_padding * 2;
	s32 width   = (itemcount - inv_offset) * (m_hotbar_imagesize + m_padding * 2);

	if (direction == HUD_DIR_TOP_BOTTOM || direction == HUD_DIR_BOTTOM_TOP) {
		s32 tmp = height;
		height = width;
		width = tmp;
	}

	// Position: screen_pos + screen_offset + alignment
	v2s32 pos(screen_offset.X * m_scale_factor, screen_offset.Y * m_scale_factor);
	pos += screen_pos;
	pos.X += (alignment.X - 1.0f) * (width * 0.5f);
	pos.Y += (alignment.Y - 1.0f) * (height * 0.5f);

	// Store hotbar_image in member variable, used by drawItem()
	if (hotbar_image != player->hotbar_image) {
		hotbar_image = player->hotbar_image;
		use_hotbar_image = !hotbar_image.empty();
	}

	// Store hotbar_selected_image in member variable, used by drawItem()
	if (hotbar_selected_image != player->hotbar_selected_image) {
		hotbar_selected_image = player->hotbar_selected_image;
		use_hotbar_selected_image = !hotbar_selected_image.empty();
	}

	// draw customized item background
	if (use_hotbar_image) {
		core::rect<s32> imgrect2(-m_padding/2, -m_padding/2,
			width+m_padding/2, height+m_padding/2);
		core::rect<s32> rect2 = imgrect2 + pos;
		video::ITexture *texture = tsrc->getTexture(hotbar_image);
		core::dimension2di imgsize(texture->getOriginalSize());
		draw2DImageFilterScaled(driver, texture, rect2,
			core::rect<s32>(core::position2d<s32>(0,0), imgsize),
			NULL, hbar_colors, true);
	}

	// Draw items
	core::rect<s32> imgrect(0, 0, m_hotbar_imagesize, m_hotbar_imagesize);
	const s32 list_max = std::min(itemcount, (s32) (mainlist ? mainlist->getSize() : 0 ));
	for (s32 i = inv_offset; i < list_max; i++) {
		s32 fullimglen = m_hotbar_imagesize + m_padding * 2;

		v2s32 steppos;
		switch (direction) {
		case HUD_DIR_RIGHT_LEFT:
			steppos = v2s32(m_padding + (list_max - 1 - i - inv_offset) * fullimglen, m_padding);
			break;
		case HUD_DIR_TOP_BOTTOM:
			steppos = v2s32(m_padding, m_padding + (i - inv_offset) * fullimglen);
			break;
		case HUD_DIR_BOTTOM_TOP:
			steppos = v2s32(m_padding, m_padding + (list_max - 1 - i - inv_offset) * fullimglen);
			break;
		default:
			steppos = v2s32(m_padding + (i - inv_offset) * fullimglen, m_padding);
			break;
		}

		core::rect<s32> item_rect = imgrect + pos + steppos;

		drawItem(mainlist->getItem(i), item_rect, (i + 1) == selectitem);

		if (is_hotbar && g_touchcontrols)
			g_touchcontrols->registerHotbarRect(i, item_rect);
	}
}

bool Hud::hasElementOfType(HudElementType type)
{
	for (size_t i = 0; i != player->maxHudId(); i++) {
		HudElement *e = player->getHud(i);
		if (!e)
			continue;
		if (e->type == type)
			return true;
	}
	return false;
}

void Hud::updateCrosshair()
{
    auto update = [this] (img::Image *img) {
        v2u size = img->getSize();
        v2u scaled_size = size * std::max(std::floor(rnd_system->getScaleFactor()), 1.0f);

        v2u wnd_size = rnd_system->getWindowSize();
        rectf r;
        r.ULC = v2f(wnd_size.X/2.0f - scaled_size.X/2.0f, wnd_size.Y/2.0f - scaled_size.Y/2.0f);
        r.LRC = r.ULC + v2f(scaled_size.X, scaled_size.Y);

        crosshair->getShape()->updateRectangle(0, r,
            {crosshair_color, crosshair_color, crosshair_color, crosshair_color});
        crosshair->updateMesh(true);
        crosshair->updateMesh(false);
    };

    std::string imgname;
    if (pointing_at_object)
        imgname = object_crosshair_img;
    else
        imgname = crosshair_img;

    auto img = cache->get<img::Image>(ResourceType::IMAGE, imgname);
    update(img);
}

void Hud::updateHUDElement(const HudElement *elem, const v3s16 &camera_offset)
{
    std::vector<HudElement *> elems(player->maxHudId());

    // Add builtin elements if the server doesn't send them.
    // Declared here such that they have the same lifetime as the elems vector
    HudElement minimap;
     HudElement hotbar;
    if (client->getProtoVersion() < 44 && (player->hud_flags & HUD_FLAG_MINIMAP_VISIBLE)) {
        minimap = {HUD_ELEM_MINIMAP, v2f(1, 0), "", v2f(), "", 0 , 0, 0, v2f(-1, 1),
                v2f(-10, 10), v3f(), v2i(256, 256), 0, "", 0};
        elems.push_back(&minimap);
    }
    if (client->getProtoVersion() < 46 && player->hud_flags & HUD_FLAG_HOTBAR_VISIBLE) {
        hotbar = {HUD_ELEM_HOTBAR, v2f(0.5, 1), "", v2f(), "", 0 , 0, 0, v2f(0, -1),
                v2f(0, -4), v3f(), v2i(), 0, "", 0};
        elems.push_back(&hotbar);
    }

    for (u32 i = 0; i < player->maxHudId(); i++)
        elems[i] = player->getHud(i);

    std::sort(elems.begin(), elems.end(), [] (const HudElement *elem1, const HudElement *elem2)
    {
        return elem1->z_index < elem2->z_index;
    })
    }
}
void Hud::drawLuaElements(const v3s16 &camera_offset)
{
	const u32 text_height = g_fontengine->getTextHeight();
	gui::IGUIFont *const font = g_fontengine->getFont();

	// Reorder elements by z_index
	std::vector<HudElement*> elems;
	elems.reserve(player->maxHudId());

	// Add builtin elements if the server doesn't send them.
	// Declared here such that they have the same lifetime as the elems vector
	HudElement minimap;
	HudElement hotbar;
	if (client->getProtoVersion() < 44 && (player->hud_flags & HUD_FLAG_MINIMAP_VISIBLE)) {
		minimap = {HUD_ELEM_MINIMAP, v2f(1, 0), "", v2f(), "", 0 , 0, 0, v2f(-1, 1),
				v2f(-10, 10), v3f(), v2s32(256, 256), 0, "", 0};
		elems.push_back(&minimap);
	}
	if (client->getProtoVersion() < 46 && player->hud_flags & HUD_FLAG_HOTBAR_VISIBLE) {
		hotbar = {HUD_ELEM_HOTBAR, v2f(0.5, 1), "", v2f(), "", 0 , 0, 0, v2f(0, -1),
				v2f(0, -4), v3f(), v2s32(), 0, "", 0};
		elems.push_back(&hotbar);
	}

	for (size_t i = 0; i != player->maxHudId(); i++) {
		HudElement *e = player->getHud(i);
		if (!e)
			continue;

		auto it = elems.begin();
		while (it != elems.end() && (*it)->z_index <= e->z_index)
			++it;

		elems.insert(it, e);
	}

	for (HudElement *e : elems) {

		v2s32 pos(floor(e->pos.X * (float) m_screensize.X + 0.5),
				floor(e->pos.Y * (float) m_screensize.Y + 0.5));
		switch (e->type) {
			case HUD_ELEM_TEXT: {
				unsigned int font_size = g_fontengine->getDefaultFontSize();

				if (e->size.X > 0)
					font_size *= e->size.X;

#ifdef __ANDROID__
				// The text size on Android is not proportional with the actual scaling
				// FIXME: why do we have such a weird unportable hack??
				if (font_size > 3 && e->offset.X < -20)
					font_size -= 3;
#endif
				auto textfont = g_fontengine->getFont(FontSpec(font_size,
					(e->style & HUD_STYLE_MONO) ? FM_Mono : FM_Unspecified,
					e->style & HUD_STYLE_BOLD, e->style & HUD_STYLE_ITALIC));

				irr::gui::CGUITTFont *ttfont = nullptr;
				if (textfont->getType() == irr::gui::EGFT_CUSTOM)
					ttfont = static_cast<irr::gui::CGUITTFont *>(textfont);

				video::SColor color(255, (e->number >> 16) & 0xFF,
										 (e->number >> 8)  & 0xFF,
										 (e->number >> 0)  & 0xFF);
				EnrichedString text(unescape_string(utf8_to_wide(e->text)), color);
				core::dimension2d<u32> textsize = textfont->getDimension(text.c_str());

				v2s32 offset(0, (e->align.Y - 1.0) * (textsize.Height / 2));
				core::rect<s32> size(0, 0, e->scale.X * m_scale_factor,
						text_height * e->scale.Y * m_scale_factor);
				v2s32 offs(e->offset.X * m_scale_factor,
						e->offset.Y * m_scale_factor);

				// Draw each line
				// See also: GUIFormSpecMenu::parseLabel
				size_t str_pos = 0;
				while (str_pos < text.size()) {
					EnrichedString line = text.getNextLine(&str_pos);

					core::dimension2d<u32> linesize = textfont->getDimension(line.c_str());
					v2s32 line_offset((e->align.X - 1.0) * (linesize.Width / 2), 0);
					if (ttfont)
						ttfont->draw(line, size + pos + offset + offs + line_offset);
					else
						textfont->draw(line.c_str(), size + pos + offset + offs + line_offset, color);
					offset.Y += linesize.Height;
				}
				break; }
			case HUD_ELEM_STATBAR: {
				v2s32 offs(e->offset.X, e->offset.Y);
				drawStatbar(pos, HUD_CORNER_UPPER, e->dir, e->text, e->text2,
					e->number, e->item, offs, e->size);
				break; }
			case HUD_ELEM_INVENTORY: {
				InventoryList *inv = inventory->getList(e->text);
				if (!inv)
					warningstream << "HUD: Unknown inventory list. name=" << e->text << std::endl;
				drawItems(pos, v2s32(e->offset.X, e->offset.Y), e->number, e->align, 0,
					inv, e->item, e->dir, false);
				break; }
			case HUD_ELEM_WAYPOINT: {
				if (!calculateScreenPos(camera_offset, e, &pos))
					break;

				pos += v2s32(e->offset.X, e->offset.Y);
				video::SColor color(255, (e->number >> 16) & 0xFF,
										 (e->number >> 8)  & 0xFF,
										 (e->number >> 0)  & 0xFF);
				std::wstring text = unescape_translate(utf8_to_wide(e->name));
				const std::string &unit = e->text;
				// Waypoints reuse the item field to store precision,
				// item = precision + 1 and item = 0 <=> precision = 10 for backwards compatibility.
				// Also see `push_hud_element`.
				u32 item = e->item;
				float precision = (item == 0) ? 10.0f : (item - 1.f);
				bool draw_precision = precision > 0;

				core::rect<s32> bounds(0, 0, font->getDimension(text.c_str()).Width, (draw_precision ? 2:1) * text_height);
				pos.Y += (e->align.Y - 1.0) * bounds.getHeight() / 2;
				bounds += pos;
				font->draw(text.c_str(), bounds + v2s32((e->align.X - 1.0) * bounds.getWidth() / 2, 0), color);
				if (draw_precision) {
					std::ostringstream os;
					v3f p_pos = player->getPosition() / BS;
					float distance = std::floor(precision * p_pos.getDistanceFrom(e->world_pos)) / precision;
					os << distance << unit;
					text = unescape_translate(utf8_to_wide(os.str()));
					bounds.LRC.X = bounds.ULC.X + font->getDimension(text.c_str()).Width;
					font->draw(text.c_str(), bounds + v2s32((e->align.X - 1.0f) * bounds.getWidth() / 2, text_height), color);
				}
				break; }
			case HUD_ELEM_IMAGE_WAYPOINT: {
				if (!calculateScreenPos(camera_offset, e, &pos))
					break;
				[[fallthrough]];
			}
			case HUD_ELEM_IMAGE: {
				video::ITexture *texture = tsrc->getTexture(e->text);
				if (!texture)
					continue;

				const video::SColor color(255, 255, 255, 255);
				const video::SColor colors[] = {color, color, color, color};
				core::dimension2di imgsize(texture->getOriginalSize());
				v2s32 dstsize(imgsize.Width * e->scale.X * m_scale_factor,
				              imgsize.Height * e->scale.Y * m_scale_factor);
				if (e->scale.X < 0)
					dstsize.X = m_screensize.X * (e->scale.X * -0.01);
				if (e->scale.Y < 0)
					dstsize.Y = m_screensize.Y * (e->scale.Y * -0.01);
				v2s32 offset((e->align.X - 1.0) * dstsize.X / 2,
				             (e->align.Y - 1.0) * dstsize.Y / 2);
				core::rect<s32> rect(0, 0, dstsize.X, dstsize.Y);
				rect += pos + offset + v2s32(e->offset.X * m_scale_factor,
				                             e->offset.Y * m_scale_factor);
				draw2DImageFilterScaled(driver, texture, rect,
					core::rect<s32>(core::position2d<s32>(0,0), imgsize),
					NULL, colors, true);
				break; }
			case HUD_ELEM_COMPASS: {
				video::ITexture *texture = tsrc->getTexture(e->text);
				if (!texture)
					continue;

				// Positionning :
				v2s32 dstsize(e->size.X, e->size.Y);
				if (e->size.X < 0)
					dstsize.X = m_screensize.X * (e->size.X * -0.01);
				if (e->size.Y < 0)
					dstsize.Y = m_screensize.Y * (e->size.Y * -0.01);

				if (dstsize.X <= 0 || dstsize.Y <= 0)
					return; // Avoid zero divides

				// Angle according to camera view
				scene::ICameraSceneNode *cam = client->getSceneManager()->getActiveCamera();
				v3f fore = cam->getAbsoluteTransformation()
						.rotateAndScaleVect(v3f(0.f, 0.f, 1.f));
				int angle = - fore.getHorizontalAngle().Y;

				// Limit angle and ajust with given offset
				angle = (angle + (int)e->number) % 360;

				core::rect<s32> dstrect(0, 0, dstsize.X, dstsize.Y);
				dstrect += pos + v2s32(
								(e->align.X - 1.0) * dstsize.X / 2,
								(e->align.Y - 1.0) * dstsize.Y / 2) +
						v2s32(e->offset.X * m_hud_scaling, e->offset.Y * m_hud_scaling);

				switch (e->dir) {
				case HUD_COMPASS_ROTATE:
					drawCompassRotate(e, texture, dstrect, angle);
					break;
				case HUD_COMPASS_ROTATE_REVERSE:
					drawCompassRotate(e, texture, dstrect, -angle);
					break;
				case HUD_COMPASS_TRANSLATE:
					drawCompassTranslate(e, texture, dstrect, angle);
					break;
				case HUD_COMPASS_TRANSLATE_REVERSE:
					drawCompassTranslate(e, texture, dstrect, -angle);
					break;
				default:
					break;
				}
				break; }
			case HUD_ELEM_MINIMAP: {
				if (!client->getMinimap())
					break;
				// Draw a minimap of size "size"
				v2s32 dstsize(e->size.X * m_scale_factor,
				              e->size.Y * m_scale_factor);

				// Only one percentage is supported to avoid distortion.
				if (e->size.X < 0)
					dstsize.X = dstsize.Y = m_screensize.X * (e->size.X * -0.01);
				else if (e->size.Y < 0)
					dstsize.X = dstsize.Y = m_screensize.Y * (e->size.Y * -0.01);

				if (dstsize.X <= 0 || dstsize.Y <= 0)
					return;

				v2s32 offset((e->align.X - 1.0) * dstsize.X / 2,
				             (e->align.Y - 1.0) * dstsize.Y / 2);
				core::rect<s32> rect(0, 0, dstsize.X, dstsize.Y);
				rect += pos + offset + v2s32(e->offset.X * m_scale_factor,
				                             e->offset.Y * m_scale_factor);
				client->getMinimap()->drawMinimap(rect);
				break; }
			case HUD_ELEM_HOTBAR: {
				drawHotbar(pos, e->offset, e->dir, e->align);
				break; }
			default:
				infostream << "Hud::drawLuaElements: ignoring drawform " << e->type
					<< " due to unrecognized type" << std::endl;
		}
	}
}

void Hud::drawHotbar(const v2s32 &pos, const v2f &offset, u16 dir, const v2f &align)
{
	if (g_touchcontrols)
		g_touchcontrols->resetHotbarRects();

	InventoryList *mainlist = inventory->getList("main");
	if (mainlist == NULL) {
		// Silently ignore this. We may not be initialized completely.
		return;
	}

	u16 playeritem = player->getWieldIndex();
	v2s32 screen_offset(offset.X, offset.Y);

	s32 hotbar_itemcount = player->getMaxHotbarItemcount();
	s32 width = hotbar_itemcount * (m_hotbar_imagesize + m_padding * 2);

	const v2u32 &window_size = RenderingEngine::getWindowSize();
	if ((float) width / (float) window_size.X <=
			g_settings->getFloat("hud_hotbar_max_width")) {
		drawItems(pos, screen_offset, hotbar_itemcount, align, 0,
			mainlist, playeritem + 1, dir, true);
	} else {
		v2s32 upper_pos = pos - v2s32(0, m_hotbar_imagesize + m_padding);

		drawItems(upper_pos, screen_offset, hotbar_itemcount / 2, align, 0,
			mainlist, playeritem + 1, dir, true);
		drawItems(pos, screen_offset, hotbar_itemcount, align,
			hotbar_itemcount / 2, mainlist, playeritem + 1, dir, true);
	}
}

void Hud::resizeHotbar() {
	const v2u32 &window_size = RenderingEngine::getWindowSize();

	if (m_screensize != window_size) {
		m_hotbar_imagesize = floor(HOTBAR_IMAGE_SIZE *
			RenderingEngine::getDisplayDensity() + 0.5);
		m_hotbar_imagesize *= m_hud_scaling;
		m_padding = m_hotbar_imagesize / 12;
		m_screensize = window_size;
		m_displaycenter = v2s32(m_screensize.X/2,m_screensize.Y/2);
	}
}

struct MeshTimeInfo {
	u64 time;
	scene::IMesh *mesh = nullptr;
};

void drawItemStack(
		video::IVideoDriver *driver,
		gui::IGUIFont *font,
		const ItemStack &item,
		const core::rect<s32> &rect,
		const core::rect<s32> *clip,
		Client *client,
		ItemRotationKind rotation_kind,
		const v3s16 &angle,
		const v3s16 &rotation_speed)
{
	static MeshTimeInfo rotation_time_infos[IT_ROT_NONE];

	if (item.empty()) {
		if (rotation_kind < IT_ROT_NONE && rotation_kind != IT_ROT_OTHER) {
			rotation_time_infos[rotation_kind].mesh = NULL;
		}
		return;
	}

	const bool enable_animations = g_settings->getBool("inventory_items_animations");

	auto *idef = client->idef();
	const ItemDefinition &def = item.getDefinition(idef);

	bool draw_overlay = false;

	const std::string inventory_image = item.getInventoryImage(idef);
	const std::string inventory_overlay = item.getInventoryOverlay(idef);

	bool has_mesh = false;
	ItemMesh *imesh;

	core::rect<s32> viewrect = rect;
	if (clip != nullptr)
		viewrect.clipAgainst(*clip);

	// Render as mesh if animated or no inventory image
	if ((enable_animations && rotation_kind < IT_ROT_NONE) || inventory_image.empty()) {
		imesh = idef->getWieldMesh(item, client);
		has_mesh = imesh && imesh->mesh;
	}
	if (has_mesh) {
		scene::IMesh *mesh = imesh->mesh;
		driver->clearBuffers(video::ECBF_DEPTH);
		s32 delta = 0;
		if (rotation_kind < IT_ROT_NONE) {
			MeshTimeInfo &ti = rotation_time_infos[rotation_kind];
			if (mesh != ti.mesh && rotation_kind != IT_ROT_OTHER) {
				ti.mesh = mesh;
				ti.time = porting::getTimeMs();
			} else {
				delta = porting::getDeltaMs(ti.time, porting::getTimeMs()) % 100000;
			}
		}
		core::rect<s32> oldViewPort = driver->getViewPort();
		core::matrix4 oldProjMat = driver->getTransform(video::ETS_PROJECTION);
		core::matrix4 oldViewMat = driver->getTransform(video::ETS_VIEW);

		core::matrix4 ProjMatrix;
		ProjMatrix.buildProjectionMatrixOrthoLH(2.0f, 2.0f, -1.0f, 100.0f);

		core::matrix4 ViewMatrix;
		ViewMatrix.buildProjectionMatrixOrthoLH(
			2.0f * viewrect.getWidth() / rect.getWidth(),
			2.0f * viewrect.getHeight() / rect.getHeight(),
			-1.0f,
			100.0f);
		ViewMatrix.setTranslation(core::vector3df(
			1.0f * (rect.LRC.X + rect.ULC.X -
					viewrect.LRC.X - viewrect.ULC.X) /
					viewrect.getWidth(),
			1.0f * (viewrect.LRC.Y + viewrect.ULC.Y -
					rect.LRC.Y - rect.ULC.Y) /
					viewrect.getHeight(),
			0.0f));

		driver->setTransform(video::ETS_PROJECTION, ProjMatrix);
		driver->setTransform(video::ETS_VIEW, ViewMatrix);

		core::matrix4 matrix;
		matrix.makeIdentity();

		if (enable_animations) {
			float timer_f = (float) delta / 5000.f;
			matrix.setRotationDegrees(v3f(
				angle.X + rotation_speed.X * 3.60f * timer_f,
				angle.Y + rotation_speed.Y * 3.60f * timer_f,
				angle.Z + rotation_speed.Z * 3.60f * timer_f)
			);
		}

		driver->setTransform(video::ETS_WORLD, matrix);
		driver->setViewPort(viewrect);

		video::SColor basecolor =
			client->idef()->getItemstackColor(item, client);

		const u32 mc = mesh->getMeshBufferCount();
		if (mc > imesh->buffer_colors.size())
			imesh->buffer_colors.resize(mc);
		for (u32 j = 0; j < mc; ++j) {
			scene::IMeshBuffer *buf = mesh->getMeshBuffer(j);
			video::SColor c = basecolor;

			auto &p = imesh->buffer_colors[j];
			p.applyOverride(c);

			// TODO: could be moved to a shader
			if (p.needColorize(c)) {
				buf->setDirty(scene::EBT_VERTEX);
				if (imesh->needs_shading)
					colorizeMeshBuffer(buf, &c);
				else
					setMeshBufferColor(buf, c);
			}

			video::SMaterial &material = buf->getMaterial();
			material.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF;
			driver->setMaterial(material);
			driver->drawMeshBuffer(buf);
		}

		driver->setTransform(video::ETS_VIEW, oldViewMat);
		driver->setTransform(video::ETS_PROJECTION, oldProjMat);
		driver->setViewPort(oldViewPort);

		draw_overlay = def.type == ITEM_NODE && inventory_image.empty();
	} else { // Otherwise just draw as 2D
		video::ITexture *texture = client->idef()->getInventoryTexture(item, client);
		video::SColor color;
		if (texture) {
			color = client->idef()->getItemstackColor(item, client);
		} else {
			color = video::SColor(255, 255, 255, 255);
			ITextureSource *tsrc = client->getTextureSource();
			texture = tsrc->getTexture("no_texture.png");
			if (!texture)
				return;
		}

		const video::SColor colors[] = { color, color, color, color };

		draw2DImageFilterScaled(driver, texture, rect,
			core::rect<s32>({0, 0}, core::dimension2di(texture->getOriginalSize())),
			clip, colors, true);

		draw_overlay = true;
	}

	// draw the inventory_overlay
	if (!inventory_overlay.empty() && draw_overlay) {
		ITextureSource *tsrc = client->getTextureSource();
		video::ITexture *overlay_texture = tsrc->getTexture(inventory_overlay);
		core::dimension2d<u32> dimens = overlay_texture->getOriginalSize();
		core::rect<s32> srcrect(0, 0, dimens.Width, dimens.Height);
		draw2DImageFilterScaled(driver, overlay_texture, rect, srcrect, clip, 0, true);
	}

	if (def.type == ITEM_TOOL && item.wear != 0) {
		// Draw a progressbar
		float barheight = static_cast<float>(rect.getHeight()) / 16;
		float barpad_x = static_cast<float>(rect.getWidth()) / 16;
		float barpad_y = static_cast<float>(rect.getHeight()) / 16;

		core::rect<s32> progressrect(
			rect.ULC.X + barpad_x,
			rect.LRC.Y - barpad_y - barheight,
			rect.LRC.X - barpad_x,
			rect.LRC.Y - barpad_y);

		// Shrink progressrect by amount of tool damage
		float wear = item.wear / 65535.0f;
		int progressmid =
			wear * progressrect.ULC.X +
			(1 - wear) * progressrect.LRC.X;

		// Compute progressbar color
		// default scheme:
		//   wear = 0.0: green
		//   wear = 0.5: yellow
		//   wear = 1.0: red

		video::SColor color;
		auto barParams = item.getWearBarParams(client->idef());
		if (barParams.has_value()) {
			f32 durabilityPercent = 1.0 - wear;
			color = barParams->getWearBarColor(durabilityPercent);
		} else {
			color = video::SColor(255, 255, 255, 255);
			int wear_i = MYMIN(std::floor(wear * 600), 511);
			wear_i = MYMIN(wear_i + 10, 511);

			if (wear_i <= 255)
				color.set(255, wear_i, 255, 0);
			else
				color.set(255, 255, 511 - wear_i, 0);
		}

		core::rect<s32> progressrect2 = progressrect;
		progressrect2.LRC.X = progressmid;
		driver->draw2DRectangle(color, progressrect2, clip);

		color = video::SColor(255, 0, 0, 0);
		progressrect2 = progressrect;
		progressrect2.ULC.X = progressmid;
		driver->draw2DRectangle(color, progressrect2, clip);
	}

	const std::string &count_text = item.metadata.getString("count_meta");
	if (font != nullptr && (item.count >= 2 || !count_text.empty())) {
		// Get the item count as a string
		std::string text = count_text.empty() ? itos(item.count) : count_text;
		v2u32 dim = font->getDimension(utf8_to_wide(unescape_enriched(text)).c_str());
		v2s32 sdim(dim.X, dim.Y);

		core::rect<s32> rect2(
			rect.LRC - sdim,
			rect.LRC
		);

		// get the count alignment
		s32 count_alignment = stoi(item.metadata.getString("count_alignment"));
		if (count_alignment != 0) {
			s32 a_x = count_alignment & 3;
			s32 a_y = (count_alignment >> 2) & 3;

			s32 x1, x2, y1, y2;
			switch (a_x) {
			case 1: // left
				x1 = rect.ULC.X;
				x2 = x1 + sdim.X;
				break;
			case 2: // middle
				x1 = (rect.ULC.X + rect.LRC.X - sdim.X) / 2;
				x2 = x1 + sdim.X;
				break;
			case 3: // right
				x2 = rect.LRC.X;
				x1 = x2 - sdim.X;
				break;
			default: // 0 = default
				x1 = rect2.ULC.X;
				x2 = rect2.LRC.X;
				break;
			}

			switch (a_y) {
			case 1: // up
				y1 = rect.ULC.Y;
				y2 = y1 + sdim.Y;
				break;
			case 2: // middle
				y1 = (rect.ULC.Y + rect.LRC.Y - sdim.Y) / 2;
				y2 = y1 + sdim.Y;
				break;
			case 3: // down
				y2 = rect.LRC.Y;
				y1 = y2 - sdim.Y;
				break;
			default: // 0 = default
				y1 = rect2.ULC.Y;
				y2 = rect2.LRC.Y;
				break;
			}

			rect2 = core::rect<s32>(x1, y1, x2, y2);
		}

		video::SColor color(255, 255, 255, 255);
		font->draw(utf8_to_wide(text).c_str(), rect2, color, false, false, &viewrect);
	}
}

void drawItemStack(
		video::IVideoDriver *driver,
		gui::IGUIFont *font,
		const ItemStack &item,
		const core::rect<s32> &rect,
		const core::rect<s32> *clip,
		Client *client,
		ItemRotationKind rotation_kind)
{
	drawItemStack(driver, font, item, rect, clip, client, rotation_kind,
		v3s16(0, 0, 0), v3s16(0, 100, 0));
}
