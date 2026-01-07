#include "nametag.h"
#include "client/core/client.h"
#include "client/core/clientenvironment.h"
#include "client/player/localplayer.h"
#include "client/player/playercamera.h"
#include "settings.h"
#include "client/render/rendersystem.h"
#include "client/ui/glyph_atlas.h"

Nametag::Nametag(Client *client,
        const std::string &text,
        const img::color8 &textcolor,
        const std::optional<img::color8> &bgcolor,
        const v3f &pos)
    : Waypoint(client, client->getEnv().getLocalPlayer()->getCamera()->getPosition()+pos*BS),
      drawBatch(std::make_unique<SpriteDrawBatch>(client->getRenderSystem(), client->getResourceCache())),
      text(text),
      textcolor(textcolor),
      bgcolor(bgcolor),
      localPos(pos)
{
    drawBatch->addTextSprite(utf8_to_wide(text));
    show_backgrounds = g_settings->getBool("show_nametag_backgrounds");
}

void Nametag::update()
{
    v3f camera_pos = client->getEnv().getLocalPlayer()->getCamera()->getPosition();

    v2f screen_pos;
    setWorldPos(camera_pos+localPos*BS);
    calculateScreenPos(&screen_pos);

    std::wstring nametag_colorless = unescape_translate(utf8_to_wide(text));

    auto font_mgr = client->getRenderSystem()->getFontManager();

    auto font = font_mgr->getFontOrCreate(render::FontMode::GRAY, render::FontStyle::NORMAL);
    v2f textsize_f = toV2T<f32>(font->getTextSize(nametag_colorless));

    auto textSprite = dynamic_cast<UITextSprite *>(drawBatch->getSprite(0));
    textSprite->getTextObj().setOverrideFont(font);
    textSprite->setText(nametag_colorless);

    auto bgcolor = getBgColor(show_backgrounds);
    if (bgcolor.A() != 0) {
        screen_pos.X -= 2.0f;
        textsize_f.X += 4.0f;
    }

    textSprite->getTextObj().setBackgroundColor(bgcolor);
    textSprite->getTextObj().setOverrideColor(textcolor);
    textSprite->setBoundRect(rectf(screen_pos-textsize_f/2.0f, textsize_f.X, textsize_f.Y));
}
