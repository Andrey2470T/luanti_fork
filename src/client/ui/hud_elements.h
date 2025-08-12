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
    ResourceCache *cache;
    RenderSystem *rnd_system;

    const HudElement *elem;
public:
    HudSprite(ResourceCache *_cache, RenderSystem *_rnd_system, const HudElement *_elem)
        : cache(_cache), rnd_system(_rnd_system), elem(_elem)
    {}
    virtual ~HudSprite() = default;

    HudElementType getType() const
    {
        return elem->type;
    }

    s16 getZIndex() const
    {
        return elem->z_index;
    }

    virtual void setVisible(bool visible) = 0;

    virtual void update() = 0;
    virtual void draw() = 0;
};

class HudText : public HudSprite
{
    std::unique_ptr<UITextSprite> text;
public:
    HudText(Client *client, const HudElement *elem);

    void setVisible(bool visible) override
    {
        text->setVisible(visible);
    }
    void update() override;
    void draw() override
    {
        text->draw();
    }
};

class HudStatbar : public HudSprite
{
    std::unique_ptr<UISprite> bar;
public:
    HudStatbar(Client *client, const HudElement *elem);

    void setVisible(bool visible) override
    {
        bar->setVisible(visible);
    }
    void update() override;
    void draw() override
    {
        bar->draw();
    }
};

// General interface fot displaying always facing sprites
class Waypoint
{
protected:
	Client *client;
    std::unique_ptr<UISpriteBank> faceBank;
    
    v3f worldPos;
public:
    Waypoint(Client *_client, UISpriteBank *_faceBank, v3f _worldPos=v3f(0.0f));
    
    virtual void updateBank(v3f newWorldPos) = 0;
    void drawBank()
    {
        faceBank->drawBank();
    }
protected:
    bool calculateScreenPos(v2f *pos);
};

// Renders an arbitrary sprite projecting it onto the screen
// If this sprite is a distance text, then displaying the current distance from the local player before it (but not mandatorily)
class HudWaypoint : public HudSprite, public Waypoint
{
    bool image;
public:
    HudWaypoint(Client *_client, const HudElement *elem, UISpriteBank *bank);

    void setVisible(bool visible) override
    {
        for (u32 i = 0; i < faceBank->getSpriteCount(); i++)
            faceBank->getSprite(i)->setVisible(visible);
    }
    void update() override
    {
    	updateBank(elem->world_pos);
    }
    void updateBank(v3f newWorldPos) override;
    void draw() override
    {
        drawBank();
    }
};

class HudImage : public HudSprite
{
    std::unique_ptr<UISprite> image;
public:
    HudImage(Client *client, const HudElement *elem);

    void setVisible(bool visible) override
    {
        image->setVisible(visible);
    }
    void update() override;
    void draw() override
    {
        image->draw();
    }
};

class HudCompass : public HudSprite
{
    Client *client;
    std::unique_ptr<UISprite> compass;

    recti viewport;
public:
    HudCompass(Client *_client, const HudElement *elem);

    void setVisible(bool visible) override
    {
        compass->setVisible(visible);
    }
    void update() override;
    void draw() override;
private:
    void updateTranslate(const rectf &r, img::Image *img, s32 angle);
    void updateRotate(const rectf &r, img::Image *img, s32 angle);
};

class Minimap;

class HudMinimap : public HudSprite
{
    Client *client;
    std::unique_ptr<Minimap> minimap;

    recti viewport;
public:
    HudMinimap(Client *_client, const HudElement *elem);

    Minimap *getUnderlyingMinimap() const
    {
        return minimap.get();
    }

    void setVisible(bool visible) override
    {
        minimap->setVisible(visible);
    }

    void update() override;
    void draw() override;
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
    Client *client;
    u32 invlistItemCount;
    std::string background_img;
    std::unique_ptr<UISpriteBank> list;
public:
    HudInventoryList(Client *_client, const HudElement *elem);
    ~HudInventoryList();

    void setInventoryList(InventoryList *_list, u32 list_offset, u32 list_itemcount)
    {
        invlist = _list;
        invlistOffset = list_offset;
        invlistItemCount = std::min(list_itemcount, invlist->getSize());
    }

    void updateBackgroundImages();
    void updateSelectedSlot(std::optional<u32> selected_index);

    void setVisible(bool visible) override
    {
        for (u32 i = 0; i < list->getSpriteCount(); i++)
            list->getSprite(i)->setVisible(visible);
    }
    void update() override;
    void draw() override
    {
    	list->drawBank();
    }

    void updateScalingSetting();
private:
    rectf getSlotRect(u32 n) const;
};

class HudHotbar : public HudInventoryList
{
public:
    HudHotbar(Client *client, const HudElement *elem, Inventory *inv);
    
    void updateSelectedSlot();
    void update() override;

    void updateMaxHotbarItemCount();
};
