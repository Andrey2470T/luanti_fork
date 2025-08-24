// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#pragma once

#include "clientenvironment.h"
#include <ostream>
#include <map>
#include <memory>
#include <set>
#include <unordered_set>
#include "gamedef.h"
#include "inventorymanager.h"
#include "gameparams.h"
#include "script/common/c_types.h" // LuaError
#include "util/numeric.h"
#include "util/string.h" // StringMap
#include "config.h"
#include "network/networkprotocol.h"

#if !IS_CLIENT_BUILD
#error Do not include in server builds
#endif

#define CLIENT_CHAT_MESSAGE_LIMIT_PER_10S 10.0f

class PlayerCamera;
class ClientMediaDownloader;
class ISoundManager;
class IWritableItemDefManager;
class MapBlockMesh;
class MapDatabase;
class MeshUpdateManager;
class Minimap;
class ModChannelMgr;
class MtEventManager;
class NodeDefManager;
class ParticleManager;
class SingleMediaDownloader;
struct ChatMessage;
struct ClientDynamicInfo;
struct ClientEvent;
struct DrawControl;
struct MapNode;
struct MeshMakeData;
struct MinimapMapblock;
struct PlayerControl;
struct PointedThing;
class RenderSystem;
class ResourceCache;
class ClientPacketHandler;

enum LocalClientState {
    LC_Created,
    LC_Init,
    LC_Ready
};

using sound_handle_t = int;

class ClientScripting;

class Client : public InventoryManager, public IGameDef
{
public:
	/*
		NOTE: Nothing is thread-safe here.
	*/

	Client(
			const char *playername,
			const std::string &password,
			IWritableItemDefManager *itemdef,
			NodeDefManager *nodedef,
			ISoundManager *sound,
            MtEventManager *event
	);

	~Client();
	DISABLE_CLASS_COPY(Client);

	// Load local mods into memory
	void scanModSubfolder(const std::string &mod_name, const std::string &mod_path,
				std::string mod_subpath);
	inline void scanModIntoMemory(const std::string &mod_name, const std::string &mod_path)
	{
		scanModSubfolder(mod_name, mod_path, "");
	}

	/*
	 request all threads managed by client to be stopped
	 */
	void Stop();


	bool isShutdown();

	/*
		Stuff that references the environment is valid only as
		long as this is not called. (eg. Players)
		If this throws a PeerNotFoundException, the connection has
		timed out.
	*/
    void step(f32 dtime);

    LocalClientState getState() { return m_state; }

	ClientEnvironment& getEnv() { return m_env; }
	ISoundManager *sound() { return getSoundManager(); }
	static const std::string &getBuiltinLuaPath();
	static const std::string &getClientModsLuaPath();

	const std::vector<ModSpec> &getMods() const override;
	const ModSpec* getModSpec(const std::string &modname) const override;

	// Causes urgent mesh updates (unlike Map::add/removeNodeWithEvent)
	void removeNode(v3s16 p);

	// helpers to enforce CSM restrictions
	MapNode CSMGetNode(v3s16 p, bool *is_valid_position);
	int CSMClampRadius(v3s16 pos, int radius);
	v3s16 CSMClampPos(v3s16 pos);

	void addNode(v3s16 p, MapNode n, bool remove_metadata = true);

	void setPlayerControl(PlayerControl &control);

	/* InventoryManager interface */
	Inventory* getInventory(const InventoryLocation &loc) override;
	void inventoryAction(InventoryAction *a) override;

	const std::set<std::string> &getConnectedPlayerNames()
	{
		return m_env.getPlayerNames();
	}

	float getAnimationTime();

    /*int getCrackLevel();
	v3s16 getCrackPos();
    void setCrack(int level, v3s16 pos);*/

	u16 getHP();

	bool checkPrivilege(const std::string &priv) const
	{ return (m_privileges.count(priv) != 0); }

	const std::unordered_set<std::string> &getPrivilegeList() const
	{ return m_privileges; }

	bool getChatMessage(std::wstring &message);
	void typeChatMessage(const std::wstring& message);

	u64 getMapSeed(){ return m_map_seed; }

    void addUpdateMeshTask(v3s16 blockpos, bool ack_to_server=false, bool urgent=false);
	// Including blocks at appropriate edges
	void addUpdateMeshTaskWithEdge(v3s16 blockpos, bool ack_to_server=false, bool urgent=false);
    void addUpdateMeshTaskForNode(v3s16 nodepos, bool ack_to_server=false, bool urgent=false);

	void updateCameraOffset(v3s16 camera_offset);

	bool hasClientEvents() const { return !m_client_event_queue.empty(); }
	// Get event from queue. If queue is empty, it triggers an assertion failure.
	ClientEvent * getClientEvent();

	bool m_simple_singleplayer_mode;

	float mediaReceiveProgress();

    //void drawLoadScreen(const std::wstring &text, float dtime, int percent);
	void afterContentReceived();
	void showUpdateProgressTexture(void *args, u32 progress, u32 max_progress);

	float getRTT();
	float getCurRate();

	// IGameDef interface
	IItemDefManager* getItemDefManager() override;
	const NodeDefManager* getNodeDefManager() override;
	ICraftDefManager* getCraftDefManager() override;
	u16 allocateUnknownNodeId(const std::string &name) override;
	virtual ISoundManager* getSoundManager();
	MtEventManager* getEventManager();
	bool checkLocalPrivilege(const std::string &priv)
	{ return checkPrivilege(priv); }
	const std::string* getModFile(std::string filename);
	ModStorageDatabase *getModStorageDatabase() override { return m_mod_storage_database; }

	// Migrates away old files-based mod storage if necessary
	void migrateModStorage();

	// The following set of functions is used by ClientMediaDownloader
	// Insert a media file appropriately into the appropriate manager
	bool loadMedia(const std::string &data, const std::string &filename,
		bool from_media_push = false);

	// Send a request for conventional media transfer
	void request_media(const std::vector<std::string> &file_requests);

    //void makeScreenshot();

	inline void pushToChatQueue(ChatMessage *cec)
	{
		m_chat_queue.push(cec);
	}

	ClientScripting *getScript() { return m_script; }
	bool modsLoaded() const { return m_mods_loaded; }

	void pushToEventQueue(ClientEvent *event);

	inline u64 getCSMRestrictionFlags() const
	{
		return m_csm_restriction_flags;
	}

	inline bool checkCSMRestrictionFlag(CSMRestrictionFlags flag) const
	{
		return m_csm_restriction_flags & flag;
	}

	ModChannel *getModChannel(const std::string &channel) override;

	const std::string &getFormspecPrepend() const;

	inline MeshGrid getMeshGrid()
	{
		return m_mesh_grid;
	}

    RenderSystem *getRenderSystem() const
    {
        return m_render_system.get();
    }

    ResourceCache *getResourceCache() const
    {
        return m_resource_cache.get();
    }

	bool inhibit_inventory_revert = false;

private:
    void initLocalMapSaving(u16 port, const std::string &hostname,
        bool is_local_server);

	void loadMods();

	void deleteAuthData();
	// helper method shared with clientpackethandler
	static AuthMechanism choseAuthMech(const u32 mechs);

    void startAuth(AuthMechanism chosen_auth_mechanism);

    bool canSendChatMessage() const;

    void sendChatMessage(const std::wstring &message);

    void clearOutChatQueue();

    void sendChangePassword(const std::string &oldpassword,
        const std::string &newpassword);

    // own state
    LocalClientState m_state = LC_Created;

	IntervalLimiter m_map_timer_and_unload_interval;

	IWritableItemDefManager *m_itemdef;
	NodeDefManager *m_nodedef;
	ISoundManager *m_sound;
	MtEventManager *m_event;
    std::unique_ptr<ResourceCache> m_resource_cache;
    std::unique_ptr<RenderSystem> m_render_system;

    std::unique_ptr<ClientPacketHandler> m_packet_handler;

	std::unique_ptr<MeshUpdateManager> m_mesh_update_manager;
	ClientEnvironment m_env;

	bool m_update_wielded_item = false;
	Inventory *m_inventory_from_server = nullptr;
	float m_inventory_from_server_age = 0.0f;

	// Block mesh animation parameters
	float m_animation_time = 0.0f;
	//int m_crack_level = -1;
	//v3s16 m_crack_pos;
	// 0 <= m_daynight_i < DAYNIGHT_CACHE_COUNT
	//s32 m_daynight_i;
	//u32 m_daynight_ratio;
	std::queue<std::wstring> m_out_chat_queue;
	u32 m_last_chat_message_sent;
	float m_chat_message_allowance = 5.0f;
	std::queue<ChatMessage *> m_chat_queue;

    float m_avg_rtt_timer = 0.0f;

	// The authentication methods we can use to enter sudo mode (=change password)
	u32 m_sudo_auth_methods;

	// The seed returned by the server in TOCLIENT_INIT is stored here
	u64 m_map_seed = 0;

	// Auth data
	std::string m_playername;
	std::string m_password;
	// If set, this will be sent (and cleared) upon a TOCLIENT_ACCEPT_SUDO_MODE
	std::string m_new_password;
	// Usable by auth mechanisms.
	AuthMechanism m_chosen_auth_mech;
	void *m_auth_data = nullptr;

	std::queue<ClientEvent *> m_client_event_queue;
	bool m_itemdef_received = false;
	bool m_nodedef_received = false;
	bool m_activeobjects_received = false;
	bool m_mods_loaded = false;

	std::vector<std::string> m_remote_media_servers;
	// Media downloader, only exists during init
	ClientMediaDownloader *m_media_downloader;
	// Pending downloads of dynamic media (key: token)
	std::vector<std::pair<u32, std::shared_ptr<SingleMediaDownloader>>> m_pending_media_downloads;

	// time_of_day speed approximation for old protocol
	bool m_time_of_day_set = false;
	float m_last_time_of_day_f = -1.0f;
	float m_time_of_day_update_timer = 0.0f;

	// Sounds
	float m_removed_sounds_check_timer = 0.0f;
	// Mapping from server sound ids to our sound ids
	std::unordered_map<s32, sound_handle_t> m_sounds_server_to_client;
	// And the other way!
	// This takes ownership for the sound handles.
	std::unordered_map<sound_handle_t, s32> m_sounds_client_to_server;
	// Relation of client id to object id
	std::unordered_map<sound_handle_t, u16> m_sounds_to_objects;

	// Privileges
	std::unordered_set<std::string> m_privileges;

	// Detached inventories
	// key = name
	std::unordered_map<std::string, Inventory*> m_detached_inventories;

	// Storage for mesh data for creating multiple instances of the same mesh
	StringMap m_mesh_data;

	// Used for saving server map to disk client-side
	MapDatabase *m_localdb = nullptr;
	IntervalLimiter m_localdb_save_interval;
	u16 m_cache_save_interval;

	// Client modding
	ClientScripting *m_script = nullptr;
	ModStorageDatabase *m_mod_storage_database = nullptr;
	float m_mod_storage_save_timer = 10.0f;
	std::vector<ModSpec> m_mods;
	StringMap m_mod_vfs;

	bool m_shutdown = false;

	// CSM restrictions byteflag
	u64 m_csm_restriction_flags = CSMRestrictionFlags::CSM_RF_NONE;
	u32 m_csm_restriction_noderange = 8;

    std::unique_ptr<ModChannelMgr> m_modchannel_mgr;

	// The number of blocks the client will combine for mesh generation.
	MeshGrid m_mesh_grid;
};
