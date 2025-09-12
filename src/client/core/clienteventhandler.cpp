#include "clienteventhandler.h"
#include "client.h"
#include "client/render/clouds.h"
#include "client/render/drawlist.h"
#include "client/render/particles.h"
#include "client/render/rendersystem.h"
#include "client/render/sky.h"
#include "client/ui/gameformspec.h"
#include "clientenvironment.h"
#include "client/player/localplayer.h"
#include "client/ao/renderCAO.h"
#include "debug.h"
#include "scripting_client.h"
#include "client/player/playercamera.h"
#include "settings.h"
#include <Image/Converting.h>

const ClientEventTable ClientEventHandler::clientEventTable = {
    &ClientEventHandler::handleClientEvent_None,
    &ClientEventHandler::handleClientEvent_PlayerDamage,
    &ClientEventHandler::handleClientEvent_PlayerForceMove,
    &ClientEventHandler::handleClientEvent_DeathscreenLegacy,
    &ClientEventHandler::handleClientEvent_ShowFormSpec,
    &ClientEventHandler::handleClientEvent_ShowLocalFormSpec,
    &ClientEventHandler::handleClientEvent_HandleParticleEvent,
    &ClientEventHandler::handleClientEvent_HandleParticleEvent,
    &ClientEventHandler::handleClientEvent_HandleParticleEvent,
    &ClientEventHandler::handleClientEvent_HudAdd,
    &ClientEventHandler::handleClientEvent_HudRemove,
    &ClientEventHandler::handleClientEvent_HudChange,
    &ClientEventHandler::handleClientEvent_SetSky,
    &ClientEventHandler::handleClientEvent_SetSun,
    &ClientEventHandler::handleClientEvent_SetMoon,
    &ClientEventHandler::handleClientEvent_SetStars,
    &ClientEventHandler::handleClientEvent_OverrideDayNigthRatio,
    &ClientEventHandler::handleClientEvent_CloudParams
};

ClientEventHandler::ClientEventHandler(Client *_client)
    : client(_client)
{

}
void ClientEventHandler::handleClientEvent_None(ClientEvent *event)
{
    FATAL_ERROR("ClientEvent type None received");
}

void ClientEventHandler::handleClientEvent_PlayerDamage(ClientEvent *event)
{
    if (client->modsLoaded())
        client->getScript()->on_damage_taken(event->player_damage.amount);

    if (!event->player_damage.effect)
        return;

    // Damage flash and hurt tilt are not used at death
    if (player->getHP() > 0) {
        LocalPlayer *player = client->getEnv().getLocalPlayer();

        f32 hp_max = player->getCAO() ?
            player->getCAO()->getProperties().hp_max : PLAYER_MAX_HP_DEFAULT;
        f32 damage_ratio = event->player_damage.amount / hp_max;

        client->getEnv().damage_flash += 95.0f + 64.f * damage_ratio;
        client->getEnv().damage_flash = MYMIN(client->getEnv().damage_flash, 127.0f);

        player->hurt_tilt_timer = 1.5f;
        player->hurt_tilt_strength =
            rangelim(damage_ratio * 5.0f, 1.0f, 4.0f);
    }

    // Play damage sound
    client->getEventManager()->put(new SimpleTriggerEvent(MtEvent::PLAYER_DAMAGE));
}

void ClientEventHandler::handleClientEvent_PlayerForceMove(ClientEvent *event)
{
    auto camera = player->getCamera();
    camera->setOrientation({event->player_force_move.pitch, event->player_force_move.yaw});
}

void ClientEventHandler::handleClientEvent_DeathscreenLegacy(ClientEvent *event)
{
    rndsys->getGameFormSpec()->showDeathFormspecLegacy();
}

void ClientEventHandler::handleClientEvent_ShowFormSpec(ClientEvent *event)
{
    rndsys->getGameFormSpec()->showFormSpec(*event->show_formspec.formspec,
        *event->show_formspec.formname);

    delete event->show_formspec.formspec;
    delete event->show_formspec.formname;
}

void ClientEventHandler::handleClientEvent_ShowLocalFormSpec(ClientEvent *event)
{
    rndsys->getGameFormSpec()->showLocalFormSpec(*event->show_formspec.formspec,
        *event->show_formspec.formname);

    delete event->show_formspec.formspec;
    delete event->show_formspec.formname;
}

void ClientEventHandler::handleClientEvent_HandleParticleEvent(ClientEvent *event)
{
    LocalPlayer *player = client->getEnv().getLocalPlayer();
    rndsys->getParticleManager()->handleParticleEvent(event, client, player);
}

void ClientEventHandler::handleClientEvent_HudAdd(ClientEvent *event)
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

void ClientEventHandler::handleClientEvent_HudRemove(ClientEvent *event)
{
    LocalPlayer *player = client->getEnv().getLocalPlayer();

    auto i = m_hud_server_to_client.find(event->hudrm.id);
    if (i != m_hud_server_to_client.end()) {
        HudElement *e = player->removeHud(i->second);
        delete e;
        m_hud_server_to_client.erase(i);
    }

}

void ClientEventHandler::handleClientEvent_HudChange(ClientEvent *event)
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

void ClientEventHandler::handleClientEvent_SetSky(ClientEvent *event)
{
    auto sky = rndsys->getSky();

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
        rndsys->getDrawList()->getDrawControl().wanted_range = g_settings->getS16("viewing_range");

    if (event->set_sky->fog_start >= 0)
        sky->setFogStart(rangelim(event->set_sky->fog_start, 0.0f, 0.99f));
    else
        sky->setFogStart(rangelim(g_settings->getFloat("fog_start"), 0.0f, 0.99f));

    sky->setFogColor(event->set_sky->fog_color);

    delete event->set_sky;
}

void ClientEventHandler::handleClientEvent_SetSun(ClientEvent *event)
{
    auto sun = rndsys->getSky()->getSun();

    sun->setVisible(event->sun_params->visible);
    sun->setTexture(event->sun_params->texture,
        event->sun_params->tonemap, texture_src);
    sun->setScale(event->sun_params->scale);
    sun->setSunriseVisible(event->sun_params->sunrise_visible);
    sun->setSunriseTexture(event->sun_params->sunrise, texture_src);
    delete event->sun_params;
}

void ClientEventHandler::handleClientEvent_SetMoon(ClientEvent *event)
{
    auto moon = rndsys->getSky()->getMoon();

    moon->setVisible(event->moon_params->visible);
    moon->setTexture(event->moon_params->texture,
        event->moon_params->tonemap, texture_src);
    moon->setScale(event->moon_params->scale);
    delete event->moon_params;
}

void ClientEventHandler::handleClientEvent_SetStars(ClientEvent *event)
{
    auto stars = rndsys->getSky()->getStars();

    stars->setVisible(event->star_params->visible);
    stars->setCount(event->star_params->count);
    stars->setColor(event->star_params->starcolor);
    stars->setScale(event->star_params->scale);
    stars->setDayOpacity(event->star_params->day_opacity);
    delete event->star_params;
}

void ClientEventHandler::handleClientEvent_OverrideDayNigthRatio(ClientEvent *event)
{
    client->getEnv().setDayNightRatioOverride(
        event->override_day_night_ratio.do_override,
        event->override_day_night_ratio.ratio_f * 1000.0f);
}

void ClientEventHandler::handleClientEvent_CloudParams(ClientEvent *event)
{
    auto clouds = rndsys->getClouds();

    clouds->setDensity(event->cloud_params.density);
    clouds->setColorBright(img::colorU32NumberToObject(event->cloud_params.color_bright));
    clouds->setColorAmbient(img::colorU32NumberToObject(event->cloud_params.color_ambient));
    clouds->setColorShadow(img::colorU32NumberToObject(event->cloud_params.color_shadow));
    clouds->setHeight(event->cloud_params.height);
    clouds->setThickness(event->cloud_params.thickness);
    clouds->setSpeed(v2f(event->cloud_params.speed_x, event->cloud_params.speed_y));
}

void ClientEventHandler::processClientEvents()
{
    while (hasClientEvents()) {
        std::unique_ptr<ClientEvent> event(getClientEvent());
        FATAL_ERROR_IF(event->type >= CLIENTEVENT_MAX, "Invalid clientevent type");
        (clientEventTable[event->type])(event.get());
    }
}
