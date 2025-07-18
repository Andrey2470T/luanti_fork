#pragma once

#include "sprite.h"
#include "client/player/localplayer.h"

class LocalPlayer;
class HudElement;
class RenderSystem;

// Renders an arbitrary sprite projecting it onto the screen
// If this sprite is a distance text, then displaying the current distance from the local player before it (but not mandatorily)
class Waypoint
{
    LocalPlayer *player;

    HudElement *hudelem;

    std::unique_ptr<UISprite> sprite;
    std::optional<u8> distance_text_i;

    bool image_waypoint;
public:
    Waypoint(LocalPlayer *_player, UISprite *_sprite, bool _image_waypoint, std::optional<u8> _distance_text_i)
        : player(_player), sprite(std::unique_ptr<UISprite>(_sprite)),
        distance_text_i(_distance_text_i), image_waypoint(_image_waypoint)
    {}

    void update(Client *client, const v3s16 &camera_offset);
    void draw()
    {
        sprite->draw();
    }
private:
    bool calculateScreenPos(RenderSystem *system, const v3s16 &camera_offset, v2i *pos);
};
