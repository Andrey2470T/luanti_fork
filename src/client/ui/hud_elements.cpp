#include "hud_elements.h"
#include "client/render/rendersystem.h"
#include "client/render/renderer.h"
#include "glyph_atlas.h"
#include "text_sprite.h"
#include "client/media/resource.h"
#include "batcher2d.h"
#include "client/client.h"
#include "../hud.h"
#include "util/numeric.h"

void calcHUDRect(RenderSystem *rnd_system, rectf &r, const HudElement *elem,
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
            return; // Avoid zero divides
    }

    v2f offset(elem->offset.X * scale_f, elem->offset.Y * scale_f);
    v2f alignment((elem->align.X - 1.0f) * (rSize.X / 2.0f), (elem->align.Y - 1.0f) * (rSize.Y / 2.0f));

    r += pos + offset + alignment;
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
    return font_mgr->getFont(render::FontMode::GRAY, style, font_size);
}

rectf getHudTextRect(RenderSystem *rnd_system, const std::string &text, const HudElement *elem, bool use_style,
    bool scale_factor, std::optional<v2f> override_pos)
{
    auto textfont = getHudTextFont(rnd_system->getFontManager(), elem, use_style);
    EnrichedString wtext = getHudWText(elem);

    v2u text_size = textfont->getTextSize(wtext.getString());
    rectf text_r(0.0f, 0.0f, text_size.X, text_size.Y);

    calcHUDRect(rnd_system, text_r, elem, scale_factor, override_pos);

    return text_r;
}

rectf getHudImageRect(ResourceCache *cache, RenderSystem *rnd_system, const std::string &imgname, const HudElement *elem,
    bool scale_factor, std::optional<v2f> override_pos)
{
    auto img = cache->getOrLoad<img::Image>(ResourceType::IMAGE, imgname)->data.get();

    v2u imgSize = img->getSize();
    rectf imgRect(0, 0, imgSize.X, imgSize.Y);

    calcHUDRect(rnd_system, imgRect, elem, scale_factor, override_pos);

    return imgRect;
}

HudText::HudText(Client *client, const HudElement *elem)
    : HudSprite(client->getResourceCache(), client->getRenderSystem(), elem)
{
    EnrichedString wtext = getHudWText(elem);
    RenderSystem *rnd_system = client->getRenderSystem();
    text = std::make_unique<UITextSprite>(rnd_system->getFontManager(), wtext,
        rnd_system->getRenderer(), client->getResourceCache());
}

void HudText::update()
{
    FontManager *font_mgr = rnd_system->getFontManager();
    text->setOverrideFont(getHudTextFont(font_mgr, elem, true));
    text->setText(getHudWText(elem));
    text->updateBuffer(getHudTextRect(rnd_system, elem->text, elem, true, true));
}

HudStatbar::HudStatbar(Client *client, const HudElement *elem)
    : HudSprite(client->getResourceCache(), client->getRenderSystem(), elem)
{
    auto stat_img = cache->get<img::Image>(ResourceType::IMAGE, elem->text)->data.get();
    auto guiTex = rnd_system->getPool(false)->getAtlasByTile(stat_img)->getTexture();
    bar = std::make_unique<UISprite>(guiTex, rnd_system->getRenderer(), cache, true);

    update();
}

void HudStatbar::update()
{
    bar->clear();

    const img::color8 color = img::white;
    const std::array<img::color8, 4> colors = {color, color, color, color};

    auto stat_img = cache->get<img::Image>(ResourceType::IMAGE, elem->text)->data.get();
    if (!stat_img)
        return;

    img::Image *stat_img_bg = nullptr;
    if (!elem->text2.empty())
        stat_img_bg = cache->get<img::Image>(ResourceType::IMAGE, elem->text2)->data.get();

    auto guiPool = rnd_system->getPool(false);
    rectf stat_img_uvs = guiPool->getTileUV(stat_img);
    rectf stat_img_bg_uvs = guiPool->getTileUV(stat_img_bg);

    f32 scale_f = rnd_system->getScaleFactor();
    v2f srcd(stat_img->getSize().X, stat_img->getSize().Y);
    v2f dstd;
    if (elem->size.isNull())
        dstd = srcd * scale_f;
    else
        dstd = v2f(elem->size.X, elem->size.Y) * scale_f;

    v2f offset = v2f(elem->offset.X, elem->offset.Y) * scale_f;

    v2u screensize = rnd_system->getWindowSize();
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
    auto shape = bar->getShape();

    for (u32 i = 0; i < elem->number / 2; i++) {
        rectf srcrect = stat_img_uvs;
        rectf dstrect(0, 0, dstd.X, dstd.Y);

        dstrect += pos;
        shape->addRectangle(dstrect, colors, srcrect);

        pos += steppos;
    }

    if (elem->number % 2 == 1) {
        // Draw half a texture
        shape->addRectangle(dsthalfrect + pos, colors, srchalfrect + stat_img_uvs.ULC);

        if (stat_img_bg && elem->item > elem->number) {
            shape->addRectangle(dsthalfrect2 + pos, colors, srchalfrect2 + stat_img_bg_uvs.ULC);
            pos += steppos;
        }
    }

    if (stat_img_bg && elem->item > elem->number) {
        // Draw "off state" textures
        s32 start_offset;
        if (elem->number % 2 == 1)
            start_offset = elem->number / 2 + 1;
        else
            start_offset = elem->number / 2;
        for (u32 i = start_offset; i < elem->item / 2; i++) {
            rectf srcrect = stat_img_uvs;
            rectf dstrect(0, 0, dstd.X, dstd.Y);

            dstrect += pos;
            shape->addRectangle(dstrect, colors, srcrect);
            pos += steppos;
        }

        if (elem->item % 2 == 1)
            shape->addRectangle(dsthalfrect + pos, colors, srchalfrect + stat_img_bg_uvs.ULC);
    }

    bar->rebuildMesh();
}

HudWaypoint::HudWaypoint(Client *_client, const HudElement *elem, UISpriteBank *bank)
    : HudSprite(_client->getResourceCache(), _client->getRenderSystem(), elem), client(_client),
    faceBank(std::unique_ptr<UISpriteBank>(bank)), image(elem->type == HUD_ELEM_IMAGE_WAYPOINT)
{
    update();
}

void HudWaypoint::update()
{
    v2f pixelPos;

    calculateScreenPos(&pixelPos);
    faceBank->setCenter(pixelPos);

    if (!image) {
        std::string text = elem->name;

        // Waypoints reuse the item field to store precision,
        // item = precision + 1 and item = 0 <=> precision = 10 for backwards compatibility.
        // Also see `push_hud_element`.
        u32 item = elem->item;
        f32 precision = (item == 0) ? 10.0f : (item - 1.f);
        bool draw_precision = precision > 0;

        if (draw_precision) {
            std::ostringstream os;
            v3f p_pos = client->getEnv().getLocalPlayer()->getPosition() / BS;
            f32 distance = std::floor(precision * p_pos.getDistanceFrom(elem->world_pos)) / precision;
            os << distance << elem->text;
            text += os.str();
        }

        FontManager *font_mgr = rnd_system->getFontManager();
        auto text_sprite = dynamic_cast<UITextSprite *>(faceBank->getSprite(0));
        text_sprite->setOverrideFont(getHudTextFont(font_mgr, elem, true));
        text_sprite->setText(EnrichedString(text));
        text_sprite->updateBuffer(getHudTextRect(rnd_system, text, elem, true, false, pixelPos));
    }
    else {
        rectf img_rect = getHudImageRect(cache, rnd_system, elem->text, elem, false, pixelPos);
        auto img_sprite = faceBank->getSprite(0);

        img::color8 color(img::PF_RGBA8, (elem->number >> 16) & 0xFF,
                          (elem->number >> 8)  & 0xFF,
                          (elem->number >> 0)  & 0xFF, 255);
        img_sprite->getShape()->updateRectangle(0, img_rect, {color, color, color, color});
        img_sprite->updateMesh(true);
        img_sprite->updateMesh(false);
    }
}

bool HudWaypoint::calculateScreenPos(v2f *pos)
{
    v3f w_pos = elem->world_pos * BS;
    w_pos -= intToFloat(client->getEnv().getCameraOffset(), BS);

    Renderer *rnd = rnd_system->getRenderer();
    matrix4 trans = rnd->getTransformMatrix(TMatrix::Projection);
    trans *= rnd->getTransformMatrix(TMatrix::View);

    f32 transformed_pos[4] = { w_pos.X, w_pos.Y, w_pos.Z, 1.0f };
    trans.multiplyWith1x4Matrix(transformed_pos);
    if (transformed_pos[3] < 0)
        return false;
    f32 zDiv = transformed_pos[3] == 0.0f ? 1.0f : 1.0f / sqrt((transformed_pos[3]));

    v2u wndSize = rnd_system->getWindowSize();
    pos->X = wndSize.X * (0.5f * transformed_pos[0] * zDiv + 0.5f);
    pos->Y = wndSize.Y * (0.5f - transformed_pos[1] * zDiv * 0.5f);
    return true;
}

HudImage::HudImage(Client *client, const HudElement *elem)
    : HudSprite(client->getResourceCache(), client->getRenderSystem(), elem)
{
    auto img = cache->get<img::Image>(ResourceType::IMAGE, elem->text)->data.get();
    auto guiTex = rnd_system->getPool(false)->getAtlasByTile(img)->getTexture();
    image = std::make_unique<UISprite>(guiTex, rnd_system->getRenderer(), cache, true);

    update();
}

void HudImage::update()
{
    rectf img_rect = getHudImageRect(cache, rnd_system, elem->text, elem, true);
    image->getShape()->updateRectangle(0, img_rect, {img::white, img::white, img::white, img::white});
    image->updateMesh(true);
}
