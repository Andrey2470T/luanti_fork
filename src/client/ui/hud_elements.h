#pragma once

#include "text_sprite.h"
#include "client/player/localplayer.h"
#include "../hud.h"
#include <Render/TTFont.h>

struct HudElement;
class UITextSprite;
class RenderSystem;
class FontManager;

void calcHudRect(RenderSystem *rnd_system, rectf &r, const HudElement *elem,
    bool scale_factor, std::optional<v2f> override_pos=std::nullopt);

EnrichedString getHudWText(const HudElement *elem);
render::TTFont *getHudTextFont(FontManager *font_mgr, const HudElement *elem, bool use_style);

rectf getHudTextRect(RenderSystem *rnd_system, const std::string &text, const HudElement *elem, bool use_style,
    bool scale_factor, std::optional<v2f> override_pos=std::nullopt);
rectf getHudImageRect(ResourceCache *cache, RenderSystem *rnd_system, const std::string &imgname, const HudElement *elem,
    bool scale_factor, std::optional<v2f> override_pos=std::nullopt);

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

    virtual void update() = 0;
    virtual void draw() = 0;
};

class HudText : public HudSprite
{
    std::unique_ptr<UITextSprite> text;
public:
    HudText(Client *client, const HudElement *elem);

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

    void update() override;
    void draw() override
    {
        bar->draw();
    }
};

// Renders an arbitrary sprite projecting it onto the screen
// If this sprite is a distance text, then displaying the current distance from the local player before it (but not mandatorily)
class HudWaypoint : public HudSprite
{
    Client *client;
    std::unique_ptr<UISpriteBank> faceBank;

    bool image;
public:
    HudWaypoint(Client *_client, const HudElement *elem, UISpriteBank *bank);

    void update() override;
    void draw() override
    {
        faceBank->drawBank();
    }
private:
    bool calculateScreenPos(v2f *pos);
};

class HudImage : public HudSprite
{
    std::unique_ptr<UISprite> image;
public:
    HudImage(Client *client, const HudElement *elem);

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

    void update() override;
    void draw() override;
};

class HudInventoryList : public HudSprite
{
    Client *client;
    InventoryList *invlist;
    u32 invlistOffset = 0;
    u32 invlistItemCount;
    std::optional<u32> selectedItemIndex;

    u32 padding;
    u32 slotSize;

    std::string background_img;
    std::string background_selected_img;

    std::unique_ptr<UISpriteBank> list;
public:
    HudInventoryList(Client *_client, const HudElement *elem);
    ~HudInventoryList();

    void setInventoryList(InventoryList *list, u32 list_offset, u32 list_itemcount)
    {
        invlist = list;
        invlistOffset = list_offset;
        invlistItemCount = list_itemcount;
    }

    void updateBackgroundImages();
    void updateSelectedSlot(std::optional<u32> selected_index);

    void update() override;
    void draw() override;

    void updateScalingSetting();
};
