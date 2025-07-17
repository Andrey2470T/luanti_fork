// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 kwolekr, Ryan Kwolek <kwolekr@minetest.net>
// Copyright (C) 2017 red-001 <red-001@outlook.ie>

#pragma once

#include <BasicIncludes.h>
#include <Render/TTFont.h>
#include <Render/Texture2D.h>
#include <Utils/AABB.h>
#include "../hud.h"

class Client;
class ResourceCache;
class Inventory;
class InventoryList;
class LocalPlayer;
struct ItemStack;

class Hud
{
public:
	img::color8 crosshair_argb;

	bool use_crosshair_image = false;
	bool use_object_crosshair_image = false;
	std::string hotbar_image = "";
	bool use_hotbar_image = false;
	std::string hotbar_selected_image = "";
	bool use_hotbar_selected_image = false;

	Hud(Client *client, LocalPlayer *player, Inventory *inventory);

	void readScalingSetting();
	~Hud();

	void drawHotbar(const v2i &pos, const v2f &offset, u16 direction, const v2f &align);
	void resizeHotbar();
	void drawCrosshair();

	bool hasElementOfType(HudElementType type);

	void drawLuaElements(const v3s16 &camera_offset);

private:
	bool calculateScreenPos(const v3s16 &camera_offset, HudElement *e, v2i *pos);
	void drawStatbar(v2i pos, u16 corner, u16 drawdir,
			const std::string &texture, const std::string& bgtexture,
			s32 count, s32 maxcount, v2i offset, v2i size = v2i());

	void drawItems(v2i screen_pos, v2i screen_offset, s32 itemcount, v2f alignment,
			s32 inv_offset, InventoryList *mainlist, u16 selectitem,
			u16 direction, bool is_hotbar);

	void drawItem(const ItemStack &item, const recti &rect, bool selected);

	void drawCompassTranslate(HudElement *e, render::Texture2D *texture,
			const recti &rect, s32 way);

	void drawCompassRotate(HudElement *e, render::Texture2D *texture,
			const recti &rect, s32 way);

	Client *client = nullptr;
	video::IVideoDriver *driver = nullptr;
	LocalPlayer *player = nullptr;
	Inventory *inventory = nullptr;
	ITextureSource *tsrc = nullptr;

	float m_hud_scaling; // cached minetest setting
	float m_scale_factor;
	v3s16 m_camera_offset;
	v2u m_screensize;
	v2i m_displaycenter;
	s32 m_hotbar_imagesize; // Takes hud_scaling into account, updated by resizeHotbar()
	s32 m_padding; // Takes hud_scaling into account, updated by resizeHotbar()
	std::array<img::color8, 4> hbar_colors;

	std::unique_ptr<MeshBuffer> m_rotation_mesh_buffer;
};

enum ItemRotationKind
{
	IT_ROT_SELECTED,
	IT_ROT_HOVERED,
	IT_ROT_DRAGGED,
	IT_ROT_OTHER,
	IT_ROT_NONE, // Must be last, also serves as number
};

void drawItemStack(
		render::TTFont *font,
		const ItemStack &item,
		const recti &rect,
		const recti *clip,
		Client *client,
		ItemRotationKind rotation_kind);

void drawItemStack(
		render::TTFont *font,
		const ItemStack &item,
		const recti &rect,
		const recti *clip,
		Client *client,
		ItemRotationKind rotation_kind,
		const v3s16 &angle,
		const v3s16 &rotation_speed);

