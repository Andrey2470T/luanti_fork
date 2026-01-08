#pragma once

#include "text_sprite.h"
#include "client/player/localplayer.h"
#include "../hud.h"
#include <Render/TTFont.h>
#include "minimap.h"

struct HudElement;
class UITextSprite;
class RenderSystem;
class FontManager;

class HudSprite
{
protected:
    Client *client;
    RenderSystem *rndsys;
    ResourceCache *cache;

    const HudElement *elem;

    UISprite *sprite;

    img::Image *img1 = nullptr;
    img::Image *img2 = nullptr;
public:
    HudSprite(Client *_client, const HudElement *_elem, UISprite *_sprite);
    virtual ~HudSprite();

    HudElementType getType() const
    {
        return elem->type;
    }

    s16 getZIndex() const
    {
        return elem->z_index;
    }

    void setVisible(bool visible)
    {
        sprite->setVisible(visible);
    }

    virtual void update() = 0;
};

class HudText : public HudSprite
{
public:
    HudText(Client *client, const HudElement *elem, SpriteDrawBatch *drawBatch);

    void update() override;
};

class HudStatbar : public HudSprite
{
public:
    HudStatbar(Client *client, const HudElement *elem, SpriteDrawBatch *drawBatch);

    void update() override;
};

// Interface for projecting some 3d world point
class Waypoint
{
protected:
    Client *client;
    v3f worldPos;
public:
    Waypoint(Client *_client, v3f _worldPos=v3f(0.0f));
    
    void setWorldPos(v3f newWorldPos);
protected:
    bool calculateScreenPos(v2f *pos);
};

// Renders an arbitrary sprite projecting it onto the screen
// If this sprite is a distance text, then displaying the current distance from the local player before it (but not mandatorily)
class HudTextWaypoint : public HudSprite, public Waypoint
{
public:
    HudTextWaypoint(Client *_client, const HudElement *elem, SpriteDrawBatch *drawBatch);

    void update() override;
};

class HudImageWaypoint : public HudSprite, public Waypoint
{
public:
    HudImageWaypoint(Client *_client, const HudElement *elem, SpriteDrawBatch *drawBatch);

    void update() override;
};

class HudImage : public HudSprite
{
public:
    HudImage(Client *client, const HudElement *elem, SpriteDrawBatch *drawBatch);

    void update() override;
};

class HudCompass : public HudSprite
{
    recti viewport;
public:
    HudCompass(Client *_client, const HudElement *elem, SpriteDrawBatch *drawBatch);

    void update() override;
private:
    void updateTranslate(const rectf &r, img::Image *img, s32 angle);
    void updateRotate(const rectf &r, img::Image *img, s32 angle);
};

class Minimap;

class HudMinimap : public HudSprite
{
    std::unique_ptr<Minimap> minimap;

    recti viewport;
public:
    HudMinimap(Client *_client, const HudElement *elem);
    ~HudMinimap();

    Minimap *getUnderlyingMinimap() const
    {
        return minimap.get();
    }

    void update() override;
    void draw();
};

class HudInventoryList : public HudSprite
{
    InventoryList *invlist;
    u32 invlistOffset = 0;
    std::optional<u32> selectedItemIndex;

    u32 padding;
    u32 slotSize;
    
    bool list_bisected = false;

    std::string background_selected_img;
protected:
    u32 invlistItemCount;
    std::string background_img;
public:
    HudInventoryList(Client *_client, const HudElement *elem, SpriteDrawBatch *drawBatch);
    ~HudInventoryList();

    void setInventoryList(InventoryList *_list, u32 list_offset, u32 list_itemcount)
    {
        invlist = _list;
        invlistOffset = list_offset;
        invlistItemCount = std::min(list_itemcount, invlist->getSize());
    }

    void updateBackgroundImages();
    void updateSelectedSlot(u32 selected_index);

    void update() override;

    void updateScalingSetting();
private:
    rectf getSlotRect(u32 n) const;
};

class HudHotbar : public HudInventoryList
{
public:
    HudHotbar(Client *client, const HudElement *elem, SpriteDrawBatch *drawBatch);
    
    void updateSelectedSlot();
    void update() override;

    void updateMaxHotbarItemCount();
};
