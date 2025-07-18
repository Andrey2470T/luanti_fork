#include "hud_elements.h"
#include "client/render/rendersystem.h"
#include "client/render/renderer.h"
#include "client/ui/text_sprite.h"
#include "client/media/resource.h"
#include "client/ui/batcher2d.h"
#include "client/client.h"
#include "../hud.h"
#include "util/numeric.h"

void Waypoint::update(Client *client, const v3s16 &camera_offset)
{
    v2i pixelPos;
    RenderSystem *system = client->getRenderSystem();
    calculateScreenPos(system, camera_offset, &pixelPos);
    pixelPos += v2i(hudelem->offset.X, hudelem->offset.Y);

    if (!image_waypoint) {
        img::color8 color(img::PF_RGBA8, (hudelem->number >> 16) & 0xFF,
                          (hudelem->number >> 8)  & 0xFF,
                          (hudelem->number >> 0)  & 0xFF, 255);

        std::wstring name = unescape_translate(utf8_to_wide(hudelem->name));
        std::string suffix = hudelem->text;

        std::wstring text = name;

        auto textSprite = dynamic_cast<UITextSprite *>(sprite.get());

        // Waypoints reuse the item field to store precision,
        // item = precision + 1 and item = 0 <=> precision = 10 for backwards compatibility.
        // Also see `push_hud_element`.
        u32 item = hudelem->item;
        f32 precision = (item == 0) ? 10.0f : (item - 1.f);
        bool draw_precision = precision > 0;

        if (draw_precision) {
            std::ostringstream os;
            v3f p_pos = player->getPosition() / BS;
            f32 distance = std::floor(precision * p_pos.getDistanceFrom(hudelem->world_pos)) / precision;
            os << distance << suffix;
            text += L"\n";
            text += unescape_translate(utf8_to_wide(os.str()));
        }
        render::TTFont *font = textSprite->getActiveFont();
        recti bounds(0, 0, font->getTextSize(text.c_str()).X, (draw_precision ? 2:1) * font->getTextHeight(text));
        pixelPos.Y += (hudelem->align.Y - 1.0) * bounds.getHeight() / 2;
        bounds += pixelPos;
        bounds += v2i((hudelem->align.X - 1.0) * bounds.getWidth() / 2, 0);
        textSprite->enableWordWrap(true);
        textSprite->setText(text);

        textSprite->updateBuffer(rectf(bounds.ULC.X, bounds.ULC.Y, bounds.LRC.X, bounds.LRC.Y));
    }
    else {
        auto img = client->getResourceCache()->getOrLoad<img::Image>(ResourceType::IMAGE, hudelem->text);

        v2u imgSize = img->getSize();
        rectf imgRect(0, 0, imgSize.X, imgSize.Y);
        imgRect += v2f(pixelPos.X, pixelPos.Y);

        sprite->getShape()->addRectangle(imgRect, {});
        Batcher2D::appendImageRectangle(sprite->getBuffer(), imgSize, rectf(0, 0, imgSize.X, imgSize.Y), imgRect, {}, false);
    }
}

bool Waypoint::calculateScreenPos(RenderSystem *system, const v3s16 &camera_offset, v2i *pos)
{
    v3f w_pos = hudelem->world_pos * BS;
    w_pos -= intToFloat(camera_offset, BS);

    Renderer *rnd = system->getRenderer();
    matrix4 trans = rnd->getTransformMatrix(TMatrix::Projection);
    trans *= rnd->getTransformMatrix(TMatrix::View);

    f32 transformed_pos[4] = { w_pos.X, w_pos.Y, w_pos.Z, 1.0f };
    trans.multiplyWith1x4Matrix(transformed_pos);
    if (transformed_pos[3] < 0)
        return false;
    f32 zDiv = transformed_pos[3] == 0.0f ? 1.0f : 1.0f / sqrt((transformed_pos[3]));

    v2u wndSize = system->getWindowSize();
    pos->X = wndSize.X * (0.5 * transformed_pos[0] * zDiv + 0.5);
    pos->Y = wndSize.Y * (0.5 - transformed_pos[1] * zDiv * 0.5);
    return true;
}
