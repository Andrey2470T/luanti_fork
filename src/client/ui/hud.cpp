// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
// Copyright (C) 2010-2013 blue42u, Jonathon Anderson <anderjon@umail.iu.edu>
// Copyright (C) 2010-2013 kwolekr, Ryan Kwolek <kwolekr@minetest.net>

#include "hud.h"
#include "settings.h"
#include "util/numeric.h"
#include "client/core/client.h"
#include "inventory.h"
#include "client/player/localplayer.h"
#include "client/player/playercamera.h"
//#include "client/render/wieldmesh.h"
#include "minimap.h"
#include "util/enriched_string.h"
#include "client/render/rendersystem.h"
#include "client/media/resource.h"
#include "batcher2d.h"
#include "client/render/atlas.h"

Hud::Hud(Client *_client)
    : client(_client), player(client->getEnv().getLocalPlayer())
{
    initCrosshair();

    if (g_settings->getBool("enable_minimap")) {
        if (client->getProtoVersion() < 44) {
            HudElement *minimap = new HudElement{HUD_ELEM_MINIMAP, v2f(1, 0), "", v2f(), "", 0 , 0, 0, v2f(-1, 1),
                v2f(-10, 10), v3f(), v2i(256, 256), 0, "", 0};
            u32 id = player->getFreeHudID();
            hudsprites[id] =  std::unique_ptr<HudSprite>(new HudMinimap(client, minimap));
            builtinMinimapID = id;
        }
    }
    if (client->getProtoVersion() < 46) {
        HudElement *hotbar = new HudElement{HUD_ELEM_HOTBAR, v2f(0.5, 1), "", v2f(), "", 0 , 0, 0, v2f(0, -1),
            v2f(0, -4), v3f(), v2i(), 0, "", 0};
        u32 id = player->getFreeHudID();
        hudsprites[id] = std::unique_ptr<HudSprite>(new HudHotbar(client, hotbar));
        builtinHotbarID = id;
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

void Hud::updateBuiltinElements()
{
    if (g_settings->getBool("enable_minimap")) {
        if (client->getProtoVersion() < 44 && player->hud_flags & HUD_FLAG_MINIMAP_VISIBLE)
            hudsprites[builtinMinimapID]->setVisible(true);
        else
            hudsprites[builtinMinimapID]->setVisible(false);
    }

    if (client->getProtoVersion() < 46 && player->hud_flags & HUD_FLAG_HOTBAR_VISIBLE)
        hudsprites[builtinHotbarID]->setVisible(true);
    else
        hudsprites[builtinHotbarID]->setVisible(false);
}

void Hud::updateInvListSelections(std::optional<u32> slotID)
{
    for (auto &elem : hudsprites)
        if (elem.second->getType() == HUD_ELEM_INVENTORY)
            dynamic_cast<HudInventoryList *>(elem.second.get())->updateSelectedSlot(slotID);
        else if (elem.second->getType() == HUD_ELEM_HOTBAR)
            dynamic_cast<HudHotbar *>(elem.second.get())->updateSelectedSlot();
}

void Hud::addHUDElement(u32 id, const HudElement *elem)
{
    auto found_elem = hudsprites.find(id);

    if (found_elem != hudsprites.end())
        return;

    switch(elem->type) {
    case HUD_ELEM_IMAGE:
        hudsprites.emplace(std::unique_ptr<HudSprite>(new HudImage(client, elem)));
        break;
    case HUD_ELEM_TEXT:
        hudsprites.emplace(std::unique_ptr<HudSprite>(new HudText(client, elem)));
        break;
    case HUD_ELEM_STATBAR:
        hudsprites.emplace(std::unique_ptr<HudSprite>(new HudStatbar(client, elem)));
        break;
    case HUD_ELEM_INVENTORY:
        hudsprites.emplace(std::unique_ptr<HudSprite>(new HudInventoryList(client, elem)));
        break;
    case HUD_ELEM_WAYPOINT: {
        UISpriteBank *bank = new UISpriteBank(rnd_system->getRenderer(), cache);
        bank->addTextSprite(rnd_system->getFontManager(), EnrichedString(), 0);
        hudsprites.emplace(std::unique_ptr<HudSprite>(new HudWaypoint(client, elem, bank)));
        break;
    }
    case HUD_ELEM_IMAGE_WAYPOINT: {
        UISpriteBank *bank = new UISpriteBank(rnd_system->getRenderer(), cache);
        bank->addImageSprite(nullptr, rectf(), 0);
        hudsprites.emplace(std::unique_ptr<HudSprite>(new HudWaypoint(client, elem, bank)));
        break;
    }
    case HUD_ELEM_COMPASS:
        hudsprites.emplace(std::unique_ptr<HudSprite>(new HudCompass(client, elem)));
        break;
    case HUD_ELEM_MINIMAP:
        hudsprites.emplace(std::unique_ptr<HudSprite>(new HudMinimap(client, elem)));
        break;
    case HUD_ELEM_HOTBAR:
        hudsprites.emplace(std::unique_ptr<HudSprite>(new HudHotbar(client, elem)));
        break;
    }
}

void Hud::updateHUDElement(u32 id)
{
    auto found_elem = hudsprites.find(id);

    if (found_elem == hudsprites.end())
        return;

    found_elem->second->update();
}

void Hud::removeHUDElement(u32 id)
{
    auto found_elem = hudsprites.find(id);

    if (found_elem == hudsprites.end())
        return;

    hudsprites.erase(found_elem);
}

Minimap *Hud::getMinimap()
{
    if (!g_settings->getBool("enable_minimap"))
        return nullptr;

    return dynamic_cast<HudMinimap *>(hudsprites[builtinMinimapID].get())->getUnderlyingMinimap();
}

void Hud::setHudVisible(bool visible)
{
    crosshair_hidden = !visible;

    for (auto &sprite : hudsprites)
        sprite.second->setVisible(visible);
}

void Hud::render()
{
    if (!crosshair_hidden)
        crosshair->draw();

    for (auto &sprite : hudsprites)
        sprite.second->draw();
}

void Hud::initCrosshair()
{
    v3f cross_color = g_settings->getV3F("crosshair_color").value_or(v3f());
    crosshair_color = img::color8(img::PF_RGBA8,
        myround(cross_color.X), myround(cross_color.Y), myround(cross_color.Z),
        g_settings->getS32("crosshair_alpha"));

    auto crosshair_image = cache->get<img::Image>(ResourceType::IMAGE, crosshair_img);
    auto guiPool = rnd_system->getPool(false);
    crosshair = std::make_unique<UISprite>(guiPool->getAtlasByTile(crosshair_image)->getTexture(),
        rnd_system->getRenderer(), cache, guiPool->getTileRect(crosshair_image), rectf(), std::array<img::color8, 4>{}, true);
}

/*struct MeshTimeInfo {
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
}*/
