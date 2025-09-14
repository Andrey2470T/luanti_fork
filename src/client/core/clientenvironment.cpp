// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2017 celeron55, Perttu Ahola <celeron55@gmail.com>

#include "client/core/chatmessanger.h"
#include "client/network/packethandler.h"
#include "client/player/interaction.h"
#include "client/render/clouds.h"
#include "client/render/renderer.h"
#include "client/render/sky.h"
#include "client/ui/gameformspec.h"
#include "client/ui/gameui.h"
#include "client/ui/minimap.h"
#include "profiler.h"
#include "util/serialize.h"
#include "util/pointedthing.h"
#include "client/core/client.h"
#include "clientenvironment.h"
//#include "clientsimpleobject.h"
#include "client/map/clientmap.h"
#include "client/player/localplayer.h"
#include "scripting_client.h"
#include "client/map/mapblockmesh.h"
#include "raycast.h"
#include "client/ao/renderCAO.h"
#include "porting.h"
#include <algorithm>
#include "client/render/rendersystem.h"
#include "client/media/resource.h"
#include "client/render/particles.h"
#include "client/render/drawlist.h"
#include "inventorymanager.h"
#include "client/render/atlas.h"
#include "util/tracy_wrapper.h"

/*
	ClientEnvironment
*/

ClientEnvironment::ClientEnvironment(Client *client):
	Environment(client),
    m_map(std::make_unique<ClientMap>(client, client->getRenderSystem()->getDrawList())),
    m_rescache(client->getResourceCache()),
    m_client(client)
{
    g_settings->registerChangedCallback("enable_fog",
        &settingChangedCallback, this);
    g_settings->registerChangedCallback("free_move",
        &settingChangedCallback, this);
}

ClientEnvironment::~ClientEnvironment()
{
	m_ao_manager.clear();

    // Delete detached inventories
    for (auto &m_detached_inventorie : m_detached_inventories) {
        delete m_detached_inventorie.second;
    }

    //for (auto &simple_object : m_simple_objects) {
    //	delete simple_object;
    //}
}

Map &ClientEnvironment::getMap()
{
	return *m_map;
}

ClientMap &ClientEnvironment::getClientMap()
{
	return *m_map;
}

void ClientEnvironment::setLocalPlayer(LocalPlayer *player)
{
	/*
		It is a failure if already is a local player
	*/
    FATAL_ERROR_IF(m_local_player,
		"Local player already allocated");

    m_local_player.reset(player);
}

void ClientEnvironment::step(f32 dtime, bool paused)
{
    TimeTaker tt_update("ClientEnvironment::step()");
    m_animation_time += dtime;
    if(m_animation_time > 60.0) m_animation_time -= 60.0;

    auto rnd_sys = m_client->getRenderSystem();

    /* Animate both GUI and 3D atlases */
    rnd_sys->getPool(false)->updateAnimatedTiles(m_animation_time);
    rnd_sys->getPool(true)->updateAnimatedTiles(m_animation_time);

    m_time_of_day_update_timer += dtime;

    updateFog();

	/* Step time of day */
	stepTimeOfDay(dtime);

    /* Calls mods callbacks */
	if (m_client->modsLoaded())
		m_script->environment_step(dtime);

    /* Step local player */
    m_local_player->step(dtime);

	// Update lighting on local player (used for wield item)
    /*u32 day_night_ratio = getDayNightRatio();
	{
		// Get node at head

		// On InvalidPositionException, use this as default
		// (day: LIGHT_SUN, night: 0)
		MapNode node_at_lplayer(CONTENT_AIR, 0x0f, 0);

		v3s16 p = lplayer->getLightPosition();
		node_at_lplayer = m_map->getNode(p);

		u16 light = getInteriorLight(node_at_lplayer, 0, m_client->ndef());
		lplayer->light_color = encode_light(light, 0); // this transfers light.alpha
		final_color_blend(&lplayer->light_color, light, day_night_ratio);
    }*/

    /* Step active objects and update lighting of them */
	bool update_lighting = m_active_object_light_update_interval.step(dtime, 0.21);
    auto cb_state = [this, dtime, update_lighting] (ClientActiveObject *cao) {
        RenderCAO *rendercao = dynamic_cast<RenderCAO *>(cao);

		// Step object
		cao->step(dtime, this);

        if (rendercao && update_lighting)
            rendercao->updateVertexColor(true);
	};

	m_ao_manager.step(dtime, cb_state);

    /* Step and handle simple objects */
    /*g_profiler->avg("ClientEnv: CSO count [#]", m_simple_objects.size());
	for (auto i = m_simple_objects.begin(); i != m_simple_objects.end();) {
		ClientSimpleObject *simple = *i;

		simple->step(dtime);
		if(simple->m_to_be_removed) {
			delete simple;
			i = m_simple_objects.erase(i);
		}
		else {
			++i;
		}
    }*/

    /* Update GameUI */
    auto gameui = rnd_sys->getGameUI();
    gameui->clearInfoText();
    gameui->updateProfilers(dtime);
    gameui->updateDebugState(m_client);

    /* Update Camera */
    auto camera = m_local_player->getCamera();
    camera->update(dtime);

    // class PlayerInteraction
    m_local_player->getInteraction()->step(dtime);
}

/*void ClientEnvironment::addSimpleObject(ClientSimpleObject *simple)
{
	m_simple_objects.push_back(simple);
}*/

GenericCAO* ClientEnvironment::getGenericCAO(u16 id)
{
	ClientActiveObject *obj = getActiveObject(id);
	if (obj && obj->getType() == ACTIVEOBJECT_TYPE_GENERIC)
		return dynamic_cast<GenericCAO*>(obj);

	return nullptr;
}

RenderCAO* ClientEnvironment::getRenderCAO(u16 id)
{
	auto generic_cao = getGenericCAO(id);
	
	if (!generic_cao)
	    return nullptr;
	return dynamic_cast<RenderCAO*>(generic_cao);
}

void ClientEnvironment::addActiveObject(u16 id, u8 type,
	const std::string &init_data)
{
    RenderCAO *obj = new RenderCAO(m_client, &m_client->getEnv());

	if (!obj) {
		infostream<<"ClientEnvironment::addActiveObject(): "
			<<"id="<<id<<" type="<<type<<": Couldn't create object"
			<<std::endl;
		return;
	}

	obj->setId(id);

	try {
		obj->initialize(init_data);
	} catch(SerializationError &e) {
		errorstream<<"ClientEnvironment::addActiveObject():"
			<<" id="<<id<<" type="<<type
			<<": SerializationError in initialize(): "
			<<e.what()
			<<": init_data="<<serializeJsonString(init_data)
			<<std::endl;
	}

    obj->addMesh();

    u16 new_id = m_ao_manager.registerObject(
        std::unique_ptr<ClientActiveObject>(dynamic_cast<ClientActiveObject *>(obj))
    ) ? obj->getId() : 0;

    if (new_id) {
        m_map->addActiveObject(new_id);
		// Final step is to update all children which are already known
		// Data provided by AO_CMD_SPAWN_INFANT
        /*const auto &children = obj->getAttachmentChildIds();
		for (auto c_id : children) {
			if (auto *o = getActiveObject(c_id))
				o->updateAttachments();
        }*/
	}
}


void ClientEnvironment::removeActiveObject(u16 id)
{
	m_ao_manager.removeObject(id);
}

void ClientEnvironment::processActiveObjectMessage(u16 id, const std::string &data)
{
	ClientActiveObject *obj = getActiveObject(id);
	if (obj == NULL) {
		infostream << "ClientEnvironment::processActiveObjectMessage():"
			<< " got message for id=" << id << ", which doesn't exist."
			<< std::endl;
		return;
	}

	try {
		obj->processMessage(data);
	} catch (SerializationError &e) {
		errorstream<<"ClientEnvironment::processActiveObjectMessage():"
			<< " id=" << id << " type=" << obj->getType()
			<< " SerializationError in processMessage(): " << e.what()
			<< std::endl;
	}
}

/*
	Client likes to call these
*/

ClientEnvEvent ClientEnvironment::getClientEnvEvent()
{
	FATAL_ERROR_IF(m_client_event_queue.empty(),
			"ClientEnvironment::getClientEnvEvent(): queue is empty");

	ClientEnvEvent event = m_client_event_queue.front();
	m_client_event_queue.pop();
	return event;
}

void ClientEnvironment::getSelectedActiveObjects(
    const line3f &shootline_on_map,
	std::vector<PointedThing> &objects,
	const std::optional<Pointabilities> &pointabilities)
{
	auto allObjects = m_ao_manager.getActiveSelectableObjects(shootline_on_map);
	const v3f line_vector = shootline_on_map.getVector();

	for (const auto &allObject : allObjects) {
		ClientActiveObject *obj = allObject.obj;
        aabbf selection_box{{0.0f, 0.0f, 0.0f}};
		if (!obj->getSelectionBox(&selection_box))
			continue;

		v3f current_intersection;
		v3f current_normal, current_raw_normal;
        const v3f rel_pos = shootline_on_map.Start - obj->getPosition();
		bool collision;
        RenderCAO* cao = dynamic_cast<RenderCAO*>(obj);
        if (cao != nullptr && cao->getProperties().rotate_selectionbox) {
            cao->updateMatrices();
            const v3f deg = cao->getAbsoluteMatrix().getRotationDegrees();
			collision = boxLineCollision(selection_box, deg,
				rel_pos, line_vector, &current_intersection, &current_normal, &current_raw_normal);
		} else {
			collision = boxLineCollision(selection_box, rel_pos, line_vector,
				&current_intersection, &current_normal);
			current_raw_normal = current_normal;
		}
		if (collision) {
			PointabilityType pointable;
			if (pointabilities) {
                if (cao->isPlayer()) {
                    pointable = pointabilities->matchPlayer(cao->getGroups()).value_or(
                            cao->getProperties().pointable);
				} else {
                    pointable = pointabilities->matchObject(cao->getName(),
                            cao->getGroups()).value_or(cao->getProperties().pointable);
				}
			} else {
                pointable = cao->getProperties().pointable;
			}
			if (pointable != PointabilityType::POINTABLE_NOT) {
				current_intersection += obj->getPosition();
				objects.emplace_back(obj->getId(), current_intersection, current_normal, current_raw_normal,
                    (current_intersection - shootline_on_map.Start).getLengthSQ(), pointable);
			}
		}
	}
}

void ClientEnvironment::pushDamageClientEnvEvent(u16 damage, bool handle_hp)
{
    ClientEnvEvent event;
    event.type = CEE_PLAYER_DAMAGE;
    event.player_damage.amount = damage;
    event.player_damage.send_to_server = handle_hp;
    m_client_event_queue.push(event);
}

v3s16 ClientEnvironment::getCameraOffset() const
{
    return m_local_player ? m_local_player->getCamera()->getOffset() : v3s16();
}

void ClientEnvironment::updateFrameTime(bool is_paused)
{
	// if paused, m_frame_time_pause_accumulator increases by dtime,
	// otherwise, m_frame_time increases by dtime
	if (is_paused) {
		m_frame_dtime = 0;
		m_frame_time_pause_accumulator = porting::getTimeMs() - m_frame_time;
	}
	else {
		auto new_frame_time = porting::getTimeMs() - m_frame_time_pause_accumulator;
		m_frame_dtime = new_frame_time - MYMAX(m_frame_time, m_frame_time_pause_accumulator);
		m_frame_time = new_frame_time;
	}
}

Inventory* ClientEnvironment::getInventory(const InventoryLocation &loc)
{
    switch(loc.type){
    case InventoryLocation::UNDEFINED:
    {}
    break;
    case InventoryLocation::CURRENT_PLAYER:
    {

        assert(m_local_player);
        return &m_local_player->inventory;
    }
    break;
    case InventoryLocation::PLAYER:
    {
        // Check if we are working with local player inventory
        LocalPlayer *player = getLocalPlayer();
        if (!player || player->getName() != loc.name)
            return NULL;
        return &player->inventory;
    }
    break;
    case InventoryLocation::NODEMETA:
    {
        NodeMetadata *meta = m_map->getNodeMetadata(loc.p);
        if(!meta)
            return NULL;
        return meta->getInventory();
    }
    break;
    case InventoryLocation::DETACHED:
    {
        if (m_detached_inventories.count(loc.name) == 0)
            return NULL;
        return m_detached_inventories[loc.name];
    }
    break;
    default:
        FATAL_ERROR("Invalid inventory location type.");
        break;
    }
    return NULL;
}

void ClientEnvironment::updateFog()
{
// Client setting only takes effect if fog distance unlimited or debug priv

    auto rndsys = m_client->getRenderSystem();
    bool enable_fog = rndsys->getSky()->getFogDistance() < 0 ||
                      m_local_player->checkPrivilege("debug") ?
                      m_cache_enable_fog : true;

    rndsys->getRenderer()->enableFog(enable_fog);
}

void ClientEnvironment::updateTimeOfDay()
{
    u32 daynight_ratio = getDayNightRatio();
    float time_brightness = decode_light_f((float)daynight_ratio / 1000.0);
    float direct_brightness;
    bool sunlight_seen;

    auto draw_control = m_client->getRenderSystem()->getDrawList()->getDrawControl();
    auto sky =  m_client->getRenderSystem()->getSky();
    // When in noclip mode force same sky brightness as above ground so you
    // can see properly
    if (draw_control.allow_noclip && m_cache_enable_free_move &&
        m_local_player->checkPrivilege("fly")) {
        direct_brightness = time_brightness;
        sunlight_seen = true;
    } else {
        float old_brightness = sky->getBrightness();
        direct_brightness = m_map->getBackgroundBrightness(MYMIN(draw_control.fog_range * 1.2, 60 * BS),
            daynight_ratio, (int)(old_brightness * 255.5), &sunlight_seen) / 255.0;
    }

    float time_of_day = getTimeOfDayF();

    static const float maxsm = 0.05f;
    static const float todsm = 0.05f;

    if (std::fabs(time_of_day - time_of_day_smooth) > maxsm &&
            std::fabs(time_of_day - time_of_day_smooth + 1.0) > maxsm &&
            std::fabs(time_of_day - time_of_day_smooth - 1.0) > maxsm)
        time_of_day_smooth = time_of_day;

    if (time_of_day_smooth > 0.8 && time_of_day < 0.2)
        time_of_day_smooth = time_of_day_smooth * (1.0 - todsm)
                + (time_of_day + 1.0) * todsm;
    else
        time_of_day_smooth = time_of_day_smooth * (1.0 - todsm)
                + time_of_day * todsm;

    sky->update(time_of_day_smooth, time_brightness, direct_brightness,
            sunlight_seen, m_local_player->getCamera()->getCameraMode(), m_local_player->getYaw(),
            m_local_player->getPitch());
}

void ClientEnvironment::updateFrame(f32 dtime, bool paused)
{
    ZoneScoped;
    TimeTaker tt_update("ClientEnvironment::updateFrame()");

    auto rnd_sys = m_client->getRenderSystem();
    /*
        Frame time
    */
    updateFrameTime(paused);

    /*
        Fog range
    */
    auto sky = rnd_sys->getSky();
    auto draw_control = rnd_sys->getDrawList()->getDrawControl();
    if (sky->getFogDistance() >= 0) {
        draw_control.wanted_range = MYMIN(draw_control.wanted_range, sky->getFogDistance());
    }
    if (draw_control.range_all && sky->getFogDistance() < 0) {
        draw_control.fog_range = FOG_RANGE_ALL;
    } else {
        draw_control.fog_range = draw_control.wanted_range * BS;
    }

    /* Update time of day */
    updateTimeOfDay();

    auto camera = m_local_player->getCamera();
    /*
        Update clouds
    */
    rnd_sys->getClouds()->update(dtime, camera, sky, draw_control.fog_range);

    /* Step particle manager */
    rnd_sys->getParticleManager()->step(dtime);

    /*
        Damage camera tilt
    */
    if (m_local_player->hurt_tilt_timer > 0.0f) {
        m_local_player->hurt_tilt_timer -= dtime * 6.0f;

        if (m_local_player->hurt_tilt_timer < 0.0f)
            m_local_player->hurt_tilt_strength = 0.0f;
    }

    /* Update profiler */
    //gameui->updateProfilerGraphs(graph);

    /*
        Update minimap pos and rotation
    */
    auto gameui = rnd_sys->getGameUI();
    auto minimap = rnd_sys->getDefaultMinimap();
    if (minimap && gameui->getFlags() & GUIF_SHOW_HUD) {
        minimap->setPos(floatToInt(m_local_player->getPosition(), BS));
        minimap->setAngle(m_local_player->getYaw());
    }

    /*
        Get chat messages from client
    */
    m_client->getChatMessanger()->updateChat(dtime);

    /*
        Inventory
    */

    if (m_local_player->getWieldIndex() != new_playeritem)
        m_client->getPacketHandler()->sendPlayerItem(new_playeritem);

    /*if (client->updateWieldedItem()) {
        // Update wielded tool
        ItemStack selected_item, hand_item;
        ItemStack &tool_item = player->getWieldedItem(&selected_item, &hand_item);
        camera->wield(tool_item);
    }*/

    /* Update ClientMap */
    m_map->step(dtime);

    //gameui->update(client, draw_control, cam, runData.pointed_old,
     //       gui_chat_console.get(), dtime);

    rnd_sys->getGameFormSpec()->update();

    /*
        ==================== Drawing begins ====================
    */
    if (rnd_sys->getWindow()->isVisible())
        rnd_sys->render();
    /*
        ==================== End scene ====================
    */

    // Damage flash is drawn in drawScene, but the timing update is done here to
    // keep dtime out of the drawing code.
    if (damage_flash > 0.0f) {
        damage_flash -= 384.0f * dtime;
    }

    g_profiler->avg("ClientEnvironment::updateFrame(): update frame [ms]", tt_update.stop(true));
}

void ClientEnvironment::settingChangedCallback(const std::string &setting_name, void *data)
{
    ((ClientEnvironment *)data)->readSettings();
}

void ClientEnvironment::readSettings()
{
    m_cache_enable_fog = g_settings->getBool("enable_fog");
    m_cache_enable_free_move = g_settings->getBool("free_move");
}
