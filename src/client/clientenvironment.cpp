// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2017 celeron55, Perttu Ahola <celeron55@gmail.com>

#include "util/serialize.h"
#include "util/pointedthing.h"
#include "client/client.h"
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

/*
	ClientEnvironment
*/

ClientEnvironment::ClientEnvironment(Client *client):
	Environment(client),
    m_map(std::make_unique<ClientMap>(client, client->getRenderSystem()->getDrawList())),
    m_rescache(client->getResourceCache()),
    m_client(client)
{
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

void ClientEnvironment::step(float dtime)
{
    m_animation_time += dtime;
    if(m_animation_time > 60.0) m_animation_time -= 60.0;

    auto rnd_sys = m_client->getRenderSystem();

    /* Animate both GUI and 3D atlases */
    rnd_sys->getPool(false)->updateAnimatedTiles(m_animation_time);
    rnd_sys->getPool(true)->updateAnimatedTiles(m_animation_time);

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

    /* Step particle manager */
    rnd_sys->getParticleManager()->step(dtime);

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

    /* Update ClientMap */
    m_map->step(dtime);
}

/*void ClientEnvironment::addSimpleObject(ClientSimpleObject *simple)
{
	m_simple_objects.push_back(simple);
}*/

GenericCAO* ClientEnvironment::getGenericCAO(u16 id)
{
	ClientActiveObject *obj = getActiveObject(id);
	if (obj && obj->getType() == ACTIVEOBJECT_TYPE_GENERIC)
		return (GenericCAO*) obj;

	return NULL;
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
