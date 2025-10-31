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
    : Waypoint(client, nullptr, client->getEnv().getLocalPlayer()->getCamera()->getPosition()+pos*BS),
      text(text),
      textcolor(textcolor),
      bgcolor(bgcolor)
{
    auto rndsys = client->getRenderSystem();
    faceBank = std::make_unique<UISpriteBank>(rndsys, client->getResourceCache());
    faceBank->addTextSprite(rndsys->getFontManager(), EnrichedString(text), 0);

    show_backgrounds = g_settings->getBool("show_nametag_backgrounds");
}

void Nametag::updateBank(v3f newWorldPos)
{
    v3f camera_pos = client->getEnv().getLocalPlayer()->getCamera()->getPosition();

    v2f screen_pos;
    worldPos = camera_pos + newWorldPos * BS;
    calculateScreenPos(&screen_pos);
    faceBank->setCenter(screen_pos);

    std::wstring nametag_colorless = unescape_translate(utf8_to_wide(text));

    auto font_mgr = client->getRenderSystem()->getFontManager();

    auto font = font_mgr->getFontOrCreate(render::FontMode::GRAY, render::FontStyle::NORMAL);
    v2u textsize = font->getTextSize(nametag_colorless.c_str());

    auto text_sprite = dynamic_cast<UITextSprite *>(faceBank->getSprite(0));
    text_sprite->setOverrideFont(font);
    text_sprite->setText(nametag_colorless);

    v2f textsize_f(textsize.X, textsize.Y);

    auto bgcolor = getBgColor(show_backgrounds);
    if (bgcolor.A() != 0) {
        screen_pos.X -= 2.0f;
        textsize_f.X += 4.0f;
    }

    text_sprite->setBackgroundColor(bgcolor);
    text_sprite->setOverrideColor(textcolor);
    text_sprite->updateBuffer(rectf(screen_pos-textsize_f/2.0f, textsize_f.X, textsize_f.Y));
}
