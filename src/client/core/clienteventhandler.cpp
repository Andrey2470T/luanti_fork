#include "clienteventhandler.h"

const ClientEventHandler ClientEventHandler::clientEventHandler[CLIENTEVENT_MAX] = {
    {&Game::handleClientEvent_None},
    {&Game::handleClientEvent_PlayerDamage},
    {&Game::handleClientEvent_PlayerForceMove},
    {&Game::handleClientEvent_DeathscreenLegacy},
    {&Game::handleClientEvent_ShowFormSpec},
    {&Game::handleClientEvent_ShowLocalFormSpec},
    {&Game::handleClientEvent_HandleParticleEvent},
    {&Game::handleClientEvent_HandleParticleEvent},
    {&Game::handleClientEvent_HandleParticleEvent},
    {&Game::handleClientEvent_HudAdd},
    {&Game::handleClientEvent_HudRemove},
    {&Game::handleClientEvent_HudChange},
    {&Game::handleClientEvent_SetSky},
    {&Game::handleClientEvent_SetSun},
    {&Game::handleClientEvent_SetMoon},
    {&Game::handleClientEvent_SetStars},
    {&Game::handleClientEvent_OverrideDayNigthRatio},
    {&Game::handleClientEvent_CloudParams},
};

void Game::handleClientEvent_None(ClientEvent *event, CameraOrientation *cam)
{
    FATAL_ERROR("ClientEvent type None received");
}

void Game::handleClientEvent_PlayerDamage(ClientEvent *event, CameraOrientation *cam)
{
    if (client->modsLoaded())
        client->getScript()->on_damage_taken(event->player_damage.amount);

    if (!event->player_damage.effect)
        return;

    // Damage flash and hurt tilt are not used at death
    if (client->getHP() > 0) {
        LocalPlayer *player = client->getEnv().getLocalPlayer();

        f32 hp_max = player->getCAO() ?
            player->getCAO()->getProperties().hp_max : PLAYER_MAX_HP_DEFAULT;
        f32 damage_ratio = event->player_damage.amount / hp_max;

        runData.damage_flash += 95.0f + 64.f * damage_ratio;
        runData.damage_flash = MYMIN(runData.damage_flash, 127.0f);

        player->hurt_tilt_timer = 1.5f;
        player->hurt_tilt_strength =
            rangelim(damage_ratio * 5.0f, 1.0f, 4.0f);
    }

    // Play damage sound
    client->getEventManager()->put(new SimpleTriggerEvent(MtEvent::PLAYER_DAMAGE));
}

void Game::handleClientEvent_PlayerForceMove(ClientEvent *event, CameraOrientation *cam)
{
    cam->camera_yaw = event->player_force_move.yaw;
    cam->camera_pitch = event->player_force_move.pitch;
}

void Game::handleClientEvent_DeathscreenLegacy(ClientEvent *event, CameraOrientation *cam)
{
    m_game_formspec.showDeathFormspecLegacy();
}

void Game::handleClientEvent_ShowFormSpec(ClientEvent *event, CameraOrientation *cam)
{
    m_game_formspec.showFormSpec(*event->show_formspec.formspec,
        *event->show_formspec.formname);

    delete event->show_formspec.formspec;
    delete event->show_formspec.formname;
}

void Game::handleClientEvent_ShowLocalFormSpec(ClientEvent *event, CameraOrientation *cam)
{
    m_game_formspec.showLocalFormSpec(*event->show_formspec.formspec,
        *event->show_formspec.formname);

    delete event->show_formspec.formspec;
    delete event->show_formspec.formname;
}

void Game::handleClientEvent_HandleParticleEvent(ClientEvent *event,
        CameraOrientation *cam)
{
    LocalPlayer *player = client->getEnv().getLocalPlayer();
    client->getParticleManager()->handleParticleEvent(event, client, player);
}

void Game::handleClientEvent_HudAdd(ClientEvent *event, CameraOrientation *cam)
{
    LocalPlayer *player = client->getEnv().getLocalPlayer();

    u32 server_id = event->hudadd->server_id;
    // ignore if we already have a HUD with that ID
    auto i = m_hud_server_to_client.find(server_id);
    if (i != m_hud_server_to_client.end()) {
        delete event->hudadd;
        return;
    }

    HudElement *e = new HudElement;
    e->type   = static_cast<HudElementType>(event->hudadd->type);
    e->pos    = event->hudadd->pos;
    e->name   = event->hudadd->name;
    e->scale  = event->hudadd->scale;
    e->text   = event->hudadd->text;
    e->number = event->hudadd->number;
    e->item   = event->hudadd->item;
    e->dir    = event->hudadd->dir;
    e->align  = event->hudadd->align;
    e->offset = event->hudadd->offset;
    e->world_pos = event->hudadd->world_pos;
    e->size      = event->hudadd->size;
    e->z_index   = event->hudadd->z_index;
    e->text2     = event->hudadd->text2;
    e->style     = event->hudadd->style;
    m_hud_server_to_client[server_id] = player->addHud(e);

    delete event->hudadd;
}

void Game::handleClientEvent_HudRemove(ClientEvent *event, CameraOrientation *cam)
{
    LocalPlayer *player = client->getEnv().getLocalPlayer();

    auto i = m_hud_server_to_client.find(event->hudrm.id);
    if (i != m_hud_server_to_client.end()) {
        HudElement *e = player->removeHud(i->second);
        delete e;
        m_hud_server_to_client.erase(i);
    }

}

void Game::handleClientEvent_HudChange(ClientEvent *event, CameraOrientation *cam)
{
    LocalPlayer *player = client->getEnv().getLocalPlayer();

    HudElement *e = nullptr;

    auto i = m_hud_server_to_client.find(event->hudchange->id);
    if (i != m_hud_server_to_client.end()) {
        e = player->getHud(i->second);
    }

    if (e == nullptr) {
        delete event->hudchange;
        return;
    }

#define CASE_SET(statval, prop, dataprop) \
    case statval: \
        e->prop = event->hudchange->dataprop; \
        break

    switch (event->hudchange->stat) {
        CASE_SET(HUD_STAT_POS, pos, v2fdata);

        CASE_SET(HUD_STAT_NAME, name, sdata);

        CASE_SET(HUD_STAT_SCALE, scale, v2fdata);

        CASE_SET(HUD_STAT_TEXT, text, sdata);

        CASE_SET(HUD_STAT_NUMBER, number, data);

        CASE_SET(HUD_STAT_ITEM, item, data);

        CASE_SET(HUD_STAT_DIR, dir, data);

        CASE_SET(HUD_STAT_ALIGN, align, v2fdata);

        CASE_SET(HUD_STAT_OFFSET, offset, v2fdata);

        CASE_SET(HUD_STAT_WORLD_POS, world_pos, v3fdata);

        CASE_SET(HUD_STAT_SIZE, size, v2s32data);

        CASE_SET(HUD_STAT_Z_INDEX, z_index, data);

        CASE_SET(HUD_STAT_TEXT2, text2, sdata);

        CASE_SET(HUD_STAT_STYLE, style, data);

        case HudElementStat_END:
            break;
    }

#undef CASE_SET

    delete event->hudchange;
}

void Game::handleClientEvent_SetSky(ClientEvent *event, CameraOrientation *cam)
{
    sky->setVisible(false);
    // Whether clouds are visible in front of a custom skybox.
    sky->setCloudsEnabled(event->set_sky->clouds);

    // Clear the old textures out in case we switch rendering type.
    sky->clearSkyboxTextures();
    // Handle according to type
    if (event->set_sky->type == "regular") {
        // Shows the mesh skybox
        sky->setVisible(true);
        // Update mesh based skybox colours if applicable.
        sky->setSkyColors(event->set_sky->sky_color);
        sky->setHorizonTint(
            event->set_sky->fog_sun_tint,
            event->set_sky->fog_moon_tint,
            event->set_sky->fog_tint_type
        );
    } else if (event->set_sky->type == "skybox" &&
            event->set_sky->textures.size() == 6) {
        // Disable the dyanmic mesh skybox:
        sky->setVisible(false);
        // Set fog colors:
        sky->setFallbackBgColor(event->set_sky->bgcolor);
        // Set sunrise and sunset fog tinting:
        sky->setHorizonTint(
            event->set_sky->fog_sun_tint,
            event->set_sky->fog_moon_tint,
            event->set_sky->fog_tint_type
        );
        // Add textures to skybox.
        for (int i = 0; i < 6; i++)
            sky->addTextureToSkybox(event->set_sky->textures[i], i, texture_src);
    } else {
        // Handle everything else as plain color.
        if (event->set_sky->type != "plain")
            infostream << "Unknown sky type: "
                << (event->set_sky->type) << std::endl;
        sky->setVisible(false);
        sky->setFallbackBgColor(event->set_sky->bgcolor);
        // Disable directional sun/moon tinting on plain or invalid skyboxes.
        sky->setHorizonTint(
            event->set_sky->bgcolor,
            event->set_sky->bgcolor,
            "custom"
        );
    }

    // Orbit Tilt:
    sky->setBodyOrbitTilt(event->set_sky->body_orbit_tilt);

    // fog
    // do not override a potentially smaller client setting.
    sky->setFogDistance(event->set_sky->fog_distance);

    // if the fog distance is reset, switch back to the client's viewing_range
    if (event->set_sky->fog_distance < 0)
        draw_control->wanted_range = g_settings->getS16("viewing_range");

    if (event->set_sky->fog_start >= 0)
        sky->setFogStart(rangelim(event->set_sky->fog_start, 0.0f, 0.99f));
    else
        sky->setFogStart(rangelim(g_settings->getFloat("fog_start"), 0.0f, 0.99f));

    sky->setFogColor(event->set_sky->fog_color);

    delete event->set_sky;
}

void Game::handleClientEvent_SetSun(ClientEvent *event, CameraOrientation *cam)
{
    sky->setSunVisible(event->sun_params->visible);
    sky->setSunTexture(event->sun_params->texture,
        event->sun_params->tonemap, texture_src);
    sky->setSunScale(event->sun_params->scale);
    sky->setSunriseVisible(event->sun_params->sunrise_visible);
    sky->setSunriseTexture(event->sun_params->sunrise, texture_src);
    delete event->sun_params;
}

void Game::handleClientEvent_SetMoon(ClientEvent *event, CameraOrientation *cam)
{
    sky->setMoonVisible(event->moon_params->visible);
    sky->setMoonTexture(event->moon_params->texture,
        event->moon_params->tonemap, texture_src);
    sky->setMoonScale(event->moon_params->scale);
    delete event->moon_params;
}

void Game::handleClientEvent_SetStars(ClientEvent *event, CameraOrientation *cam)
{
    sky->setStarsVisible(event->star_params->visible);
    sky->setStarCount(event->star_params->count);
    sky->setStarColor(event->star_params->starcolor);
    sky->setStarScale(event->star_params->scale);
    sky->setStarDayOpacity(event->star_params->day_opacity);
    delete event->star_params;
}

void Game::handleClientEvent_OverrideDayNigthRatio(ClientEvent *event,
        CameraOrientation *cam)
{
    client->getEnv().setDayNightRatioOverride(
        event->override_day_night_ratio.do_override,
        event->override_day_night_ratio.ratio_f * 1000.0f);
}

void Game::handleClientEvent_CloudParams(ClientEvent *event, CameraOrientation *cam)
{
    clouds->setDensity(event->cloud_params.density);
    clouds->setColorBright(video::SColor(event->cloud_params.color_bright));
    clouds->setColorAmbient(video::SColor(event->cloud_params.color_ambient));
    clouds->setColorShadow(video::SColor(event->cloud_params.color_shadow));
    clouds->setHeight(event->cloud_params.height);
    clouds->setThickness(event->cloud_params.thickness);
    clouds->setSpeed(v2f(event->cloud_params.speed_x, event->cloud_params.speed_y));
}

void Game::processClientEvents(CameraOrientation *cam)
{
    while (client->hasClientEvents()) {
        std::unique_ptr<ClientEvent> event(client->getClientEvent());
        FATAL_ERROR_IF(event->type >= CLIENTEVENT_MAX, "Invalid clientevent type");
        const ClientEventHandler& evHandler = clientEventHandler[event->type];
        (this->*evHandler.handler)(event.get(), cam);
    }
}
