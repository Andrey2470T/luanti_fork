#include "hud_elements.h"
#include "client/render/rendersystem.h"
#include "client/render/renderer.h"
#include "glyph_atlas.h"
#include "gui/IGUIEnvironment.h"
#include "text_sprite.h"
#include "extra_images.h"
#include "client/media/resource.h"
#include "batcher2d.h"
#include "client/core/client.h"
#include "../hud.h"
#include "util/numeric.h"
#include "client/player/playercamera.h"
#include <Utils/Quaternion.h>
#include "minimap.h"
#include "settings.h"
#include "gui/touchcontrols.h"

v2f calcHUDOffset(RenderSystem *rnd_system, rectf &r, const HudElement *elem,
    bool scale_factor, std::optional<v2f> override_pos)
{
    v2u screensize = rnd_system->getWindowSize();
    v2f pos = override_pos.has_value() ? override_pos.value() :
                  v2f(floor(elem->pos.X * (f32)screensize.X + 0.5), floor(elem->pos.Y * (f32)screensize.Y + 0.5));

    f32 scale_f = scale_factor ? rnd_system->getScaleFactor() : 1.0f;

    v2f rSize = r.getSize();

    if (elem->type == HUD_ELEM_TEXT || elem->type == HUD_ELEM_IMAGE) {
        rSize *= v2f(elem->scale.X * scale_f, elem->scale.Y * scale_f);

        if (elem->type == HUD_ELEM_IMAGE) {
            if (elem->scale.X < 0)
                rSize.X = screensize.X * (elem->scale.X * -0.01);
            if (elem->scale.Y < 0)
                rSize.Y = screensize.Y * (elem->scale.Y * -0.01);
        }
    }

    if (elem->type == HUD_ELEM_COMPASS || elem->type == HUD_ELEM_MINIMAP) {
        rSize *= v2f(elem->size.X * scale_f, elem->size.Y * scale_f);
        if (elem->size.X < 0)
            rSize.X = screensize.X * (elem->size.X * -0.01);
        if (elem->size.Y < 0)
            rSize.Y = screensize.Y * (elem->size.Y * -0.01);

        if (rSize.X <= 0 || rSize.Y <= 0)
            return v2f(); // Avoid zero divides
    }

    v2f offset(elem->offset.X * scale_f, elem->offset.Y * scale_f);
    v2f alignment((elem->align.X - 1.0f) * (rSize.X / 2.0f), (elem->align.Y - 1.0f) * (rSize.Y / 2.0f));

    return pos + offset + alignment;
}

EnrichedString getHudWText(const HudElement *elem)
{
    img::color8 color(img::PF_RGBA8, (elem->number >> 16) & 0xFF,
                      (elem->number >> 8)  & 0xFF, (elem->number >> 0)  & 0xFF, 255);
    return EnrichedString(elem->text, color);
}

render::TTFont *getHudTextFont(FontManager *font_mgr, const HudElement *elem, bool use_style)
{
    u32 font_size = font_mgr->getDefaultFontSize(render::FontMode::GRAY);

    if (elem->size.X > 0)
        font_size *= elem->size.X;

#ifdef __ANDROID__
    // The text size on Android is not proportional with the actual scaling
    // FIXME: why do we have such a weird unportable hack??
    if (font_size > 3 && elem->offset.X < -20)
        font_size -= 3;
#endif
    render::FontStyle style = render::FontStyle::NORMAL;

    if (use_style) {
        if (elem->style & HUD_STYLE_BOLD)
            style = render::FontStyle::BOLD;
        else if (elem->style & HUD_STYLE_ITALIC)
            style = render::FontStyle::ITALIC;
    }
    return font_mgr->getFontOrCreate(render::FontMode::GRAY, style, font_size);
}

rectf getHudTextRect(RenderSystem *rnd_system, const std::string &text, const HudElement *elem, bool use_style,
    bool scale_factor, std::optional<v2f> override_pos=std::nullopt)
{
    auto textfont = getHudTextFont(rnd_system->getFontManager(), elem, use_style);
    EnrichedString wtext = getHudWText(elem);

    v2u text_size = textfont->getTextSize(wtext.getString());
    rectf text_r(0.0f, 0.0f, text_size.X, text_size.Y);

    text_r += calcHUDOffset(rnd_system, text_r, elem, scale_factor, override_pos);

    return text_r;
}

rectf getHudImageRect(ResourceCache *cache, RenderSystem *rnd_system, const std::string &imgname, const HudElement *elem,
    bool scale_factor, img::Image **img, std::optional<v2f> override_pos=std::nullopt)
{
    *img = cache->getOrLoad<img::Image>(ResourceType::IMAGE, imgname);

    v2u imgSize = (*img)->getSize();
    rectf imgRect(0, 0, imgSize.X, imgSize.Y);

    imgRect += calcHUDOffset(rnd_system, imgRect, elem, scale_factor, override_pos);

    return imgRect;
}

HudSprite::HudSprite(Client *_client, const HudElement *_elem, UISprite *_sprite)
    : client(_client), rndsys(client->getRenderSystem()), cache(client->getResourceCache()),
      elem(_elem), sprite(_sprite)
{}

HudSprite::~HudSprite()
{
    cache->clearResource<img::Image>(ResourceType::IMAGE, img1);
    cache->clearResource<img::Image>(ResourceType::IMAGE, img2);
}

HudText::HudText(Client *client, const HudElement *elem, SpriteDrawBatch *drawBatch)
    : HudSprite(client, elem, drawBatch->addTextSprite(L"", elem->z_index))
{
    update();
}

void HudText::update()
{
    FontManager *font_mgr = rndsys->getFontManager();

    auto textSprite = dynamic_cast<UITextSprite *>(sprite);
    textSprite->getTextObj().setOverrideFont(getHudTextFont(font_mgr, elem, true));

    EnrichedString textstr = getHudWText(elem);
    textSprite->getTextObj().setOverrideColor(textstr.getColors().at(0));
    textSprite->setText(textstr);
    textSprite->setBoundRect(getHudTextRect(rndsys, elem->text, elem, true, true));
}

HudStatbar::HudStatbar(Client *client, const HudElement *elem, SpriteDrawBatch *drawBatch)
    : HudSprite(client, elem, drawBatch->addRectsSprite({}, elem->z_index))
{
    update();
}

void HudStatbar::update()
{
    sprite->clear();

    img::color8 color = img::white;
    RectColors colors = {color, color, color, color};

    img1 = cache->getOrLoad<img::Image>(ResourceType::IMAGE, elem->text);
    if (!img1)
        return;

    if (!elem->text2.empty())
        img2 = cache->getOrLoad<img::Image>(ResourceType::IMAGE, elem->text2);

    auto guiPool = rndsys->getPool(false);
    rectf stat_img_src = guiPool->getTileRect(img1, false, true);
    rectf stat_img_bg_src = guiPool->getTileRect(img2, false, true);

    f32 scale_f = rndsys->getScaleFactor();
    v2f srcd(img1->getSize().X, img1->getSize().Y);
    v2f dstd;
    if (elem->size.isNull())
        dstd = srcd * scale_f;
    else
        dstd = v2f(elem->size.X, elem->size.Y) * scale_f;

    v2f offset = v2f(elem->offset.X, elem->offset.Y) * scale_f;

    v2u screensize = rndsys->getWindowSize();
    v2f pos = v2f(floor(elem->pos.X * (f32)screensize.X + 0.5), floor(elem->pos.Y * (f32)screensize.Y + 0.5));
    pos += offset;

    v2f steppos;
    switch (elem->dir) {
    case HUD_DIR_RIGHT_LEFT:
        steppos = v2f(-1.0f, 0.0f);
        break;
    case HUD_DIR_TOP_BOTTOM:
        steppos = v2f(0.0f, 1.0f);
        break;
    case HUD_DIR_BOTTOM_TOP:
        steppos = v2f(0.0f, -1.0f);
        break;
    default:
        // From left to right
        steppos = v2f(1.0f, 0.0f);
        break;
    }

    auto calculate_clipping_rect = [] (v2f src, v2f steppos) -> rectf {
        // Create basic rectangle
        rectf rect(0, 0,
                   src.X  - std::abs(steppos.X) * src.X / 2,
                   src.Y - std::abs(steppos.Y) * src.Y / 2
                   );
        // Move rectangle left or down
        if (steppos.X == -1)
            rect += v2f(src.X / 2, 0);
        if (steppos.Y == -1)
            rect += v2f(0, src.Y / 2);
        return rect;
    };
    // Rectangles for 1/2 the actual value to display
    rectf srchalfrect, dsthalfrect;
    // Rectangles for 1/2 the "off state" texture
    rectf srchalfrect2, dsthalfrect2;

    if (elem->number % 2 == 1 || elem->item % 2 == 1) {
        // Need to draw halves: Calculate rectangles
        srchalfrect  = calculate_clipping_rect(srcd, steppos);
        dsthalfrect  = calculate_clipping_rect(dstd, steppos);
        srchalfrect2 = calculate_clipping_rect(srcd, steppos * -1);
        dsthalfrect2 = calculate_clipping_rect(dstd, steppos * -1);
    }

    steppos.X *= dstd.X;
    steppos.Y *= dstd.Y;

    // Draw full textures
    auto statbarSprite = dynamic_cast<UIRects *>(sprite);

    for (u32 i = 0; i < elem->number / 2; i++) {
        rectf dstrect(0, 0, dstd.X, dstd.Y);

        dstrect += pos;
        statbarSprite->addRect({dstrect, colors, img1});

        pos += steppos;
    }

    if (elem->number % 2 == 1) {
        // Draw half a texture
        statbarSprite->addRect({dsthalfrect + pos, colors, img1}, srchalfrect + stat_img_src.ULC);

        if (img2 && elem->item > elem->number) {
            statbarSprite->addRect({dsthalfrect2 + pos, colors, img2}, srchalfrect2 + stat_img_bg_src.ULC);
            pos += steppos;
        }
    }

    if (img2 && elem->item > elem->number) {
        // Draw "off state" textures
        s32 start_offset;
        if (elem->number % 2 == 1)
            start_offset = elem->number / 2 + 1;
        else
            start_offset = elem->number / 2;
        for (u32 i = start_offset; i < elem->item / 2; i++) {
            rectf dstrect(0, 0, dstd.X, dstd.Y);

            dstrect += pos;
            statbarSprite->addRect({dstrect, colors, img1});
            pos += steppos;
        }

        if (elem->item % 2 == 1)
            statbarSprite->addRect({dsthalfrect + pos, colors, img1}, srchalfrect + stat_img_src.ULC);
    }
}

Waypoint::Waypoint(Client *_client, v3f _worldPos)
    : client(_client), worldPos(_worldPos)
{}

void Waypoint::setWorldPos(v3f newWorldPos)
{
    worldPos = newWorldPos * BS;
    worldPos -= intToFloat(client->getEnv().getCameraOffset(), BS);
}

bool Waypoint::calculateScreenPos(v2f *pos)
{
    RenderSystem *rnd_system = client->getRenderSystem();
    Renderer *rnd = rnd_system->getRenderer();
    matrix4 trans = rnd->getTransformMatrix(TMatrix::Projection);
    trans *= rnd->getTransformMatrix(TMatrix::View);

    f32 transformed_pos[4] = { worldPos.X, worldPos.Y, worldPos.Z, 1.0f };
    trans.multiplyWith1x4Matrix(transformed_pos);
    if (transformed_pos[3] < 0)
        return false;
    f32 zDiv = transformed_pos[3] == 0.0f ? 1.0f : 1.0f / sqrt((transformed_pos[3]));

    v2u wndSize = rnd_system->getWindowSize();
    pos->X = wndSize.X * (0.5f * transformed_pos[0] * zDiv + 0.5f);
    pos->Y = wndSize.Y * (0.5f - transformed_pos[1] * zDiv * 0.5f);
    return true;
}

HudTextWaypoint::HudTextWaypoint(Client *_client, const HudElement *elem, SpriteDrawBatch *drawBatch)
    : HudSprite(_client, elem, drawBatch->addTextSprite(L"", elem->z_index)),
      Waypoint(_client)
{
    update();
}

void HudTextWaypoint::update()
{
    v2f pixelPos;
    calculateScreenPos(&pixelPos);

    sprite->clear();

    std::string text = elem->name;

    // Waypoints reuse the item field to store precision,
    // item = precision + 1 and item = 0 <=> precision = 10 for backwards compatibility.
    // Also see `push_hud_element`.
    u32 item = elem->item;
    f32 precision = (item == 0) ? 10.0f : (item - 1.f);
    bool draw_precision = precision > 0;

    if (draw_precision) {
        std::ostringstream os;
        v3f p_pos = HudSprite::client->getEnv().getLocalPlayer()->getPosition() / BS;
        f32 distance = std::floor(precision * p_pos.getDistanceFrom(elem->world_pos)) / precision;
        os << distance << elem->text;
        text += os.str();
    }

    FontManager *font_mgr = rndsys->getFontManager();
    auto textSprite = dynamic_cast<UITextSprite *>(sprite);
    textSprite->getTextObj().setOverrideFont(getHudTextFont(font_mgr, elem, true));
    textSprite->setText(EnrichedString(text));
    textSprite->setBoundRect(getHudTextRect(rndsys, text, elem, true, false, pixelPos));
}

HudImageWaypoint::HudImageWaypoint(Client *_client, const HudElement *elem, SpriteDrawBatch *drawBatch)
    : HudSprite(_client, elem, drawBatch->addRectsSprite({}, elem->z_index)), Waypoint(_client)
{
    update();
}

void HudImageWaypoint::update()
{
    v2f pixelPos;
    calculateScreenPos(&pixelPos);

    auto imgSprite = dynamic_cast<UIRects *>(sprite);

    rectf img_rect = getHudImageRect(cache, rndsys, elem->text, elem, false, &img1, pixelPos);

    img::color8 color(img::PF_RGBA8, (elem->number >> 16) & 0xFF,
        (elem->number >> 8)  & 0xFF,
        (elem->number >> 0)  & 0xFF, 255);

    imgSprite->updateRect(0, {img_rect, color, img1});
}

HudImage::HudImage(Client *client, const HudElement *elem, SpriteDrawBatch *drawBatch)
    : HudSprite(client, elem, drawBatch->addRectsSprite({}, elem->z_index))
{
    update();
}

void HudImage::update()
{
    rectf img_rect = getHudImageRect(cache, rndsys, elem->text, elem, true, &img1);

    auto imgSprite = dynamic_cast<UIRects *>(sprite);
    imgSprite->updateRect(0, {img_rect, RectColors::defaultColors, img1});
}

HudCompass::HudCompass(Client *_client, const HudElement *elem, SpriteDrawBatch *drawBatch)
    : HudSprite(_client, elem, drawBatch->addRectsSprite({}, elem->z_index))
{
    update();
}

void HudCompass::update()
{
    sprite->clear();

    img1 = cache->getOrLoad<img::Image>(ResourceType::IMAGE, elem->text);

    rectf destrect;
    destrect += calcHUDOffset(rndsys, destrect, elem, false, std::nullopt);

    // Angle according to camera view
    matrix4 rotation;
    v3f dir = client->getEnv().getLocalPlayer()->getCamera()->getDirection();
    Quaternion(dir.X, dir.Y, dir.Z, 1.0f).getMatrix(rotation);

    v3f fore(0.0f, 0.0f, 1.0f);
    rotation.transformVect(fore);
    int angle = - fore.getHorizontalAngle().Y;

    // Limit angle and ajust with given offset
    angle = (angle + (int)elem->number) % 360;

    switch (elem->dir) {
    case HUD_COMPASS_ROTATE:
        updateRotate(destrect, img1, angle);
        break;
    case HUD_COMPASS_ROTATE_REVERSE:
        updateRotate(destrect, img1, -angle);
        break;
    case HUD_COMPASS_TRANSLATE:
        updateTranslate(destrect, img1, angle);
        break;
    case HUD_COMPASS_TRANSLATE_REVERSE:
        updateTranslate(destrect, img1, -angle);
        break;
    default:
        break;
    }
}

/*void HudCompass::draw()
{
    auto context = rnd_system->getRenderer()->getContext();
    recti prevViewport = context->getViewportSize();

    if (elem->dir == HUD_COMPASS_ROTATE || elem->dir == HUD_COMPASS_ROTATE_REVERSE)
        context->setViewportSize(viewport);

    compass->draw();

    context->setViewportSize(prevViewport);
}*/

void HudCompass::updateTranslate(const rectf &r, img::Image *img, s32 angle)
{
    const RectColors colors;

    // Compute source image scaling

    v2u imgsize = img->getSize();
    v2f destsize(r.getHeight() * elem->scale.X * imgsize.X / imgsize.Y,
                  r.getHeight() * elem->scale.Y);

    // Avoid infinite loop
    if (destsize.X <= 0 || destsize.Y <= 0)
        return;

    rectf tgtrect(0, 0, destsize.X, destsize.Y);
    tgtrect +=  v2f(
        (r.getWidth() - destsize.X) / 2,
        (r.getHeight() - destsize.Y) / 2) +
        r.ULC;

    s32 offset = angle * destsize.X / 360;

    tgtrect += v2f(offset, 0);

    // Repeat image as much as needed
    while (tgtrect.ULC.X > r.ULC.X)
        tgtrect -= v2f(destsize.X, 0);

    auto compassSprite = dynamic_cast<UIRects *>(sprite);
    compassSprite->addRect({tgtrect, colors, img});
    tgtrect += v2f(destsize.X, 0);

    while (tgtrect.ULC.X < r.LRC.X) {
        compassSprite->addRect({tgtrect, colors, img});
        tgtrect += v2f(destsize.X, 0);
    }

    compassSprite->setClipRect(recti(r.ULC.X, r.ULC.Y, r.LRC.X, r.LRC.Y));
}

void HudCompass::updateRotate(const rectf &r, img::Image *img, s32 angle)
{
    viewport = recti(r.ULC.X, r.ULC.Y, r.LRC.X, r.LRC.Y);

    v3f dest_ulc = v3f(-1.0f, -1.0f, 0.0f);
    v3f dest_lrc = v3f(1.0f, 1.0f, 0.0f);

    matrix4 rotation;
    rotation.setRotationDegrees(v3f(0.0f, 0.0f, angle));

    rotation.transformVect(dest_ulc);
    rotation.transformVect(dest_lrc);

    rectf destrect(dest_ulc.X, dest_ulc.Y, dest_lrc.X, dest_lrc.Y);

    auto compassSprite = dynamic_cast<UIRects *>(sprite);
    compassSprite->addRect({destrect, RectColors::defaultColors, img});
}

HudMinimap::HudMinimap(Client *_client, const HudElement *elem)
    : HudSprite(_client, elem, new UIRects(_client->getResourceCache(), nullptr,
        _client->getRenderSystem()->getPool(false), 1, elem->z_index)),
    minimap(std::make_unique<Minimap>(_client, dynamic_cast<UIRects *>(sprite)))
{}

HudMinimap::~HudMinimap()
{
    delete sprite;
}

void HudMinimap::update()
{
    rectf destrect;
    destrect += calcHUDOffset(rndsys, destrect, elem, true, std::nullopt);

    auto minimapSprite = dynamic_cast<UIRects *>(sprite);
    minimapSprite->updateRect(0, {destrect, RectColors::defaultColors}, rectf(0, 1, 1, 0));

    viewport = recti(destrect.ULC.X, destrect.ULC.Y, destrect.LRC.X, destrect.LRC.Y);
    minimap->updateActiveMarkers(viewport);
}

void HudMinimap::draw()
{
    if (sprite->isVisible())
        minimap->drawMinimap(viewport);
}

static void setting_changed_callback(const std::string &name, void *data)
{
    static_cast<HudInventoryList*>(data)->updateScalingSetting();
}

HudInventoryList::HudInventoryList(Client *_client, const HudElement *elem, SpriteDrawBatch *drawBatch,
    InventoryList *_invlist)
    : HudSprite(_client, elem, drawBatch->addRectsSprite({}, elem->z_index))
{
    updateScalingSetting();
    g_settings->registerChangedCallback("dpi_change_notifier", setting_changed_callback, this);
    g_settings->registerChangedCallback("display_density_factor", setting_changed_callback, this);
    g_settings->registerChangedCallback("hud_scaling", setting_changed_callback, this);

    setInventoryList(_invlist, 0, _invlist->getSize());
    updateBackgroundImages();
}

HudInventoryList::~HudInventoryList()
{
    g_settings->deregisterAllChangedCallbacks(this);
}

void HudInventoryList::updateBackgroundImages()
{
    LocalPlayer *player = client->getEnv().getLocalPlayer();

    if (background_img != player->hotbar_image)
        background_img = player->hotbar_image;

    if (background_selected_img != player->hotbar_selected_image)
        background_selected_img = player->hotbar_selected_image;

    update();
}

void HudInventoryList::updateSelectedSlot(u32 selected_index)
{
    update();

    auto invSprite = dynamic_cast<UIRects *>(sprite);

    rectf selected_r = getSlotRect(selected_index);
    
    if (list_bisected && selected_index <= invlistItemCount / 2)
        selected_r -= v2f(0, slotSize + padding);

    if (!background_selected_img.empty()) {
    	auto selected_img = cache->getOrLoad<img::Image>(ResourceType::IMAGE, background_selected_img);
    
        selected_r.ULC -= padding*2;
        selected_r.LRC += padding*2;

        invSprite->addRect({selected_r, RectColors::defaultColors, selected_img});
    } else {
        img::color8 c_outside(img::blue);

        f32 x1 = selected_r.ULC.X;
        f32 y1 = selected_r.ULC.Y;
        f32 x2 = selected_r.LRC.X;
        f32 y2 = selected_r.LRC.Y;
        // Black base borders
        invSprite->addRect({
            rectf(
                v2f(x1 - padding, y1 - padding),
                v2f(x2 + padding, y1)
            ), {c_outside, c_outside, c_outside, c_outside}});
        invSprite->addRect({
            rectf(
                v2f(x1 - padding, y2),
                v2f(x2 + padding, y2 + padding)
            ), {c_outside, c_outside, c_outside, c_outside}});
        invSprite->addRect({
            rectf(
                v2f(x1 - padding, y1),
                v2f(x1, y2)
            ), {c_outside, c_outside, c_outside, c_outside}});
        invSprite->addRect({
            rectf(
                v2f(x2, y1),
                v2f(x2 + padding, y2)
            ), {c_outside, c_outside, c_outside, c_outside}});
    }

    //drawItemStack(driver, g_fontengine->getFont(), item, rect, NULL,
    //	client, selected ? IT_ROT_SELECTED : IT_ROT_NONE);
}

void HudInventoryList::update()
{
    u32 height  = slotSize + padding * 2;
    u32 width   = (invlistItemCount - invlistOffset) * (slotSize + padding * 2);

    if (elem->dir == HUD_DIR_TOP_BOTTOM || elem->dir == HUD_DIR_BOTTOM_TOP)
        std::swap(width, height);

    rectf listrect(0, 0, width, height);
    v2f list_offset = calcHUDOffset(rndsys, listrect, elem, true, std::nullopt);

    sprite->clear();

    auto invSprite = dynamic_cast<UIRects *>(sprite);

    RectColors colors;
    
    v2u wnd_size = rndsys->getWindowSize();
	list_bisected = width / wnd_size.X <= g_settings->getFloat("hud_hotbar_max_width");

    v2f shift_up(0, slotSize + padding);

    img::color8 color(img::PF_RGBA8, 0, 0, 0, 128);

    if (!background_img.empty()) {
    	auto img = cache->getOrLoad<img::Image>(ResourceType::IMAGE, background_img);
    
        rectf destrect(-padding/2, -padding/2, width+padding/2, height+padding/2);
        destrect += list_offset;

        if (!list_bisected)
            invSprite->addRect({destrect, colors, img});
        else {
            rectf shifted_destrect = destrect - shift_up;
            shifted_destrect.LRC.X -= shifted_destrect.getWidth()/2;
            destrect.ULC.X += destrect.getWidth()/2;
            
            invSprite->addRect({shifted_destrect, colors, img});
            invSprite->addRect({destrect, colors, img});
        }
    }
    else {
    	if (!list_bisected) {
            for (u32 i = invlistOffset; i < invlistItemCount; i++) {
                rectf destrect = getSlotRect(i);
                destrect += list_offset;

                invSprite->addRect({destrect, color});
            }
        }
        else {
            for (u32 i = invlistOffset; i < invlistItemCount / 2; i++) {
                rectf destrect = getSlotRect(i);
                destrect += list_offset;
                destrect -= shift_up;

                invSprite->addRect({destrect, color});
            }
            for (u32 i = invlistItemCount / 2; i < invlistItemCount; i++) {
                rectf destrect = getSlotRect(i);
                destrect += list_offset;

                invSprite->addRect({destrect, color});
            }
        }
    }
}

void HudInventoryList::updateScalingSetting()
{
    slotSize = floor(HOTBAR_IMAGE_SIZE * rndsys->getScaleFactor() + 0.5);
    padding = slotSize / 12;
}

rectf HudInventoryList::getSlotRect(u32 n) const
{
    assert(invlistOffset <= n && n <= invlistItemCount);
	
    u32 list_max = invlistItemCount;

    s32 fullimglen = slotSize + padding * 2;

    v2f steppos;
    switch (elem->dir) {
        case HUD_DIR_RIGHT_LEFT:
            steppos = v2f(padding + ((f32)list_max - 1 - n - (f32)invlistOffset) * fullimglen, padding);
            break;
        case HUD_DIR_TOP_BOTTOM:
            steppos = v2f(padding, padding + (n - (f32)invlistOffset) * fullimglen);
            break;
        case HUD_DIR_BOTTOM_TOP:
            steppos = v2f(padding, padding + ((f32)list_max - 1 - n - (f32)invlistOffset) * fullimglen);
            break;
        default:
            steppos = v2f(padding + (n - (f32)invlistOffset) * fullimglen, padding);
            break;
    }

   rectf slotrect(0, 0, slotSize, slotSize);
   slotrect += steppos;

   return slotrect;
}

HudHotbar::HudHotbar(Client *client, const HudElement *elem, SpriteDrawBatch *drawBatch)
    : HudInventoryList(client, elem, drawBatch, client->getEnv().getLocalPlayer()->inventory.getList("main"))
{
    if (!invlist) {
        // Silently ignore this. We may not be initialized completely.
        return;
    }

    //updateMaxHotbarItemCount();
}

void HudHotbar::updateSelectedSlot()
{
    HudInventoryList::updateSelectedSlot(client->getEnv().getLocalPlayer()->getWieldIndex());
}

void HudHotbar::update()
{
    if (g_touchcontrols)
        g_touchcontrols->resetHotbarRects();

    HudInventoryList::update();

    if (g_touchcontrols && background_img.empty()) {
        auto &shape = sprite->getShape();
        for (u32 i = 0; i < shape.getPrimitiveCount(); i++) {
            auto rect = shape.getPrimitiveArea(i);

            g_touchcontrols->registerHotbarRect(i, recti(rect.ULC.X, rect.ULC.Y, rect.LRC.X, rect.LRC.Y));
        }
    }
}

void HudHotbar::updateMaxHotbarItemCount()
{
    invlistItemCount = client->getEnv().getLocalPlayer()->getMaxHotbarItemcount();
}

	
