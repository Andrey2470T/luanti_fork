// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2017 celeron55, Perttu Ahola <celeron55@gmail.com>

#pragma once

#include "environment.h"
#include "util/numeric.h" // IntervalLimiter
#include "client/ao/clientActiveObjectMgr.h" // client::ActiveObjectMgr
#include "client/ao/clientActiveObject.h"
#include "config.h"
#include <set>

#if !IS_CLIENT_BUILD
#error Do not include in server builds
#endif

//class ClientSimpleObject;
class Client;
class ClientMap;
class ClientScripting;
class GenericCAO;
class RenderCAO;
class LocalPlayer;
class ResourceCache;
class Inventory;
struct InventoryLocation;

/*
	The client-side environment.

	This is not thread-safe.
	Must be called from main (irrlicht) thread (uses the SceneManager)
	Client uses an environment mutex.
*/

enum ClientEnvEventType
{
	CEE_NONE,
	CEE_PLAYER_DAMAGE
};

struct ClientEnvEvent
{
	ClientEnvEventType type;
	union {
		//struct{
		//} none;
		struct{
			u16 amount;
			bool send_to_server;
		} player_damage;
	};
};

typedef std::unordered_map<u16, ClientActiveObject*> ClientActiveObjectMap;
class ClientEnvironment : public Environment
{
public:
    ClientEnvironment(Client *client);
	~ClientEnvironment();

    Map & getMap() override;
	ClientMap & getClientMap();

	Client *getGameDef() { return m_client; }
	void setScript(ClientScripting *script) { m_script = script; }

    void step(f32 dtime) override {}
    void step(f32 dtime, bool paused);

	virtual void setLocalPlayer(LocalPlayer *player);
    LocalPlayer *getLocalPlayer() const { return m_local_player.get(); }

	/*
		ClientSimpleObjects
	*/

	//void addSimpleObject(ClientSimpleObject *simple);

	/*
		ActiveObjects
	*/

	GenericCAO* getGenericCAO(u16 id);
	RenderCAO* getRenderCAO(u16 id);
	ClientActiveObject* getActiveObject(u16 id)
	{
		return m_ao_manager.getActiveObject(id);
	}

	/*
		Adds an active object to the environment.
		Environment handles deletion of object.
		Object may be deleted by environment immediately.
		If id of object is 0, assigns a free id to it.
		Returns the id of the object.
		Returns 0 if not added and thus deleted.
	*/
	void addActiveObject(u16 id, u8 type, const std::string &init_data);
	void removeActiveObject(u16 id);

	void processActiveObjectMessage(u16 id, const std::string &data);

	/*
		Client likes to call these
	*/

	// Get all nearby objects
	void getActiveObjects(const v3f &origin, f32 max_d,
		std::vector<DistanceSortedActiveObject> &dest)
	{
		return m_ao_manager.getActiveObjects(origin, max_d, dest);
	}

	bool hasClientEnvEvents() const { return !m_client_event_queue.empty(); }

	// Get event from queue. If queue is empty, it triggers an assertion failure.
	ClientEnvEvent getClientEnvEvent();

    void pushDamageClientEnvEvent(u16 damage, bool handle_hp);

	virtual void getSelectedActiveObjects(
        const line3f &shootline_on_map,
		std::vector<PointedThing> &objects,
		const std::optional<Pointabilities> &pointabilities
	);

	const std::set<std::string> &getPlayerNames() { return m_player_names; }
	void addPlayerName(const std::string &name) { m_player_names.insert(name); }
	void removePlayerName(const std::string &name) { m_player_names.erase(name); }
    v3s16 getCameraOffset() const;

	void updateFrameTime(bool is_paused);
	u64 getFrameTime() const { return m_frame_time; }
	u64 getFrameTimeDelta() const { return m_frame_dtime; }

    float getAnimationTime()
    {
        return m_animation_time;
    }

    Inventory* getInventory(const InventoryLocation &loc);

    std::unordered_map<std::string, Inventory*> getDetachedInventories()
    {
        return m_detached_inventories;
    }

    void updateFog();
    void updateTimeOfDay();

    void updateFrame(f32 dtime, bool paused);

    // time_of_day speed approximation for old protocol
    bool m_time_of_day_set = false;
    f32 m_last_time_of_day_f = -1.0f;
    f32 m_time_of_day_update_timer = 0.0f;

    f32 damage_flash;

    u16 new_playeritem;
private:
    static void settingChangedCallback(const std::string &setting_name, void *data);
    void readSettings();

	std::unique_ptr<ClientMap> m_map;
	std::unique_ptr<LocalPlayer> m_local_player;

	ResourceCache *m_rescache;
	Client *m_client;
	ClientScripting *m_script = nullptr;

	client::ActiveObjectMgr m_ao_manager;
	//std::vector<ClientSimpleObject*> m_simple_objects;
	std::queue<ClientEnvEvent> m_client_event_queue;
	IntervalLimiter m_active_object_light_update_interval;
	std::set<std::string> m_player_names;

	u64 m_frame_time = 0;
	u64 m_frame_dtime = 0;
	u64 m_frame_time_pause_accumulator = 0;

    f32 time_of_day_smooth;

    // Block mesh animation parameters
    f32 m_animation_time = 0.0f;

    // Detached inventories
    // key = name
    std::unordered_map<std::string, Inventory*> m_detached_inventories;

    bool m_cache_enable_fog;
    bool m_cache_enable_free_move;
};
