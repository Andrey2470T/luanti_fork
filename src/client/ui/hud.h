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
#include "hud_elements.h"

class Client;
class RenderSystem;
class ResourceCache;
class Inventory;
class InventoryList;
class LocalPlayer;
struct ItemStack;
class UISprite;
class EnrichedString;
class AtlasPool;
class HudSprite;

class Hud
{
	Client *client = nullptr;
	RenderSystem *rnd_system;
	LocalPlayer *player = nullptr;
	Inventory *inventory = nullptr;
	ResourceCache *cache = nullptr;

    // Crosshair is not controlled by mods yet
    std::unique_ptr<UISprite> crosshair;

    class HudSpriteSorter
    {
    public:
        HudSpriteSorter() = default;

        bool operator()(const std::unique_ptr<HudSprite> &s1, const std::unique_ptr<HudSprite> &s2)
        {
            return s1->getZIndex() < s2->getZIndex();
        }
    };

    std::map<u32, std::unique_ptr<HudSprite>, HudSpriteSorter> hudsprites;
    /*std::map<u32, UP<UISprite>> images;
	std::map<u32, UP<UITextSprite>> texts;
	std::map<u32, UP<UISprite>> statbars;
	std::map<u32, UP<UIInvList>> inventories;
    std::map<u32, UP<Waypoint>> waypoints;
    std::map<u32, UP<UISprite>> compasses;
	std::map<u32, UP<Minimap>> minimaps;
    std::map<u32, UP<UIHotbar>> hotbars;*/

	v3s16 camera_offset;
	s32 hotbar_imagesize; // Takes hud_scaling into account, updated by resizeHotbar()
	s32 padding; // Takes hud_scaling into account, updated by resizeHotbar()
	std::array<img::color8, 4> hbar_colors;	
public:
    // Crosshair
    img::color8 crosshair_color;
    const std::string crosshair_img = "crosshair.png";
    const std::string object_crosshair_img = "object_crosshair.png";
    bool pointing_at_object;

	std::string hotbar_image = "";
	bool use_hotbar_image = false;
	std::string hotbar_selected_image = "";
	bool use_hotbar_selected_image = false;

    Hud(Client *_client, Inventory *_inventory);

	void readScalingSetting();
	~Hud();

	void drawHotbar(const v2i &pos, const v2f &offset, u16 direction, const v2f &align);
	void resizeHotbar();

	bool hasElementOfType(HudElementType type);

    void updateCrosshair();

    void addHUDElement(const HudElement *elem, const v3s16 &camera_offset);
    void removeHUDElement(const HudElement *elem);
    void updateHUDElement(const HudElement *elem, const v3s16 &camera_offset);
    //void drawLuaElements(const v3s16 &camera_offset);

private:
	void drawItems(v2i screen_pos, v2i screen_offset, s32 itemcount, v2f alignment,
			s32 inv_offset, InventoryList *mainlist, u16 selectitem,
			u16 direction, bool is_hotbar);

	void drawItem(const ItemStack &item, const recti &rect, bool selected);
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

