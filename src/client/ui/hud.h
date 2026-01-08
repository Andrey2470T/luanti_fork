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
class InventoryList;
class LocalPlayer;
struct ItemStack;
class UISprite;
class EnrichedString;
class AtlasPool;
class HudSprite;
class SpriteDrawBatch;

class Hud
{
    Client *client;
    LocalPlayer *player;
    ResourceCache *cache;

    SpriteDrawBatch *drawBatch;

    // Crosshair is not controlled by mods yet
    UIRects *crosshair;

    u32 builtinMinimapID;
    u32 builtinHotbarID;

    std::vector<std::pair<u32, std::unique_ptr<HudSprite>>> hudsprites;
public:
    // Crosshair
    img::color8 crosshair_color;
    const std::string crosshair_img = "crosshair.png";
    const std::string object_crosshair_img = "object_crosshair.png";

    Hud(Client *_client, SpriteDrawBatch *_drawBatch);

	bool hasElementOfType(HudElementType type);

    void updateCrosshair();
    void updateBuiltinElements();
    void updateInvListSelections(u32 slotID);

    void addHUDElement(u32 id, const HudElement *elem);
    void removeHUDElement(u32 id);
    void updateHUDElement(u32 id);

    Minimap *getMinimap();

    void setHudVisible(bool visible);

    void renderMinimaps();
private:
    HudSprite *findSprite(u32 id);
    void initCrosshair();
    void resortElements();
};

enum ItemRotationKind
{
	IT_ROT_SELECTED,
	IT_ROT_HOVERED,
	IT_ROT_DRAGGED,
	IT_ROT_OTHER,
	IT_ROT_NONE, // Must be last, also serves as number
};

/*void drawItemStack(
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
        const v3s16 &rotation_speed);*/

