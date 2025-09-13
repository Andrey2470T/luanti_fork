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
#include <Core/MainWindow.h>
#include "client/sound/soundmaker.h"

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
class ClientEventHandler;
class ChatMessanger;
class InputHandler;
class GameInputSystem;

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
            ResourceCache *resource_cache,
            RenderSystem *render_system,
            InputHandler *input,
            const char *playername,
            const std::string &password,
            ELoginRegister allow_login_or_register
    );

	~Client();
	DISABLE_CLASS_COPY(Client);

    bool initSound();

    bool shouldShowTouchControls();
    bool initGui();
    void updateSound(f32 dtime);

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

	// helpers to enforce CSM restrictions
	MapNode CSMGetNode(v3s16 p, bool *is_valid_position);
	int CSMClampRadius(v3s16 pos, int radius);
	v3s16 CSMClampPos(v3s16 pos);

	const std::set<std::string> &getConnectedPlayerNames()
	{
		return m_env.getPlayerNames();
	}

    /*int getCrackLevel();
	v3s16 getCrackPos();
    void setCrack(int level, v3s16 pos);*/

    u64 getMapSeed(){ return m_map_seed; }

	bool m_simple_singleplayer_mode;

	float mediaReceiveProgress();

	void afterContentReceived();
	void showUpdateProgressTexture(void *args, u32 progress, u32 max_progress);

	// IGameDef interface
	IItemDefManager* getItemDefManager() override;
	const NodeDefManager* getNodeDefManager() override;
	ICraftDefManager* getCraftDefManager() override;
	u16 allocateUnknownNodeId(const std::string &name) override;
	virtual ISoundManager* getSoundManager();
    SoundMaker *getSoundMaker() const
    {
        return m_soundmaker.get();
    }
	MtEventManager* getEventManager();
	const std::string* getModFile(std::string filename);
	ModStorageDatabase *getModStorageDatabase() override { return m_mod_storage_database; }

	// Migrates away old files-based mod storage if necessary
	void migrateModStorage();

	// The following set of functions is used by ClientMediaDownloader
	// Insert a media file appropriately into the appropriate manager
	bool loadMedia(const std::string &data, const std::string &filename,
		bool from_media_push = false);

    //void makeScreenshot();

	ClientScripting *getScript() { return m_script; }
	bool modsLoaded() const { return m_mods_loaded; }

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

    RenderSystem *getRenderSystem() const
    {
        return m_render_system;
    }

    ResourceCache *getResourceCache() const
    {
        return m_resource_cache;
    }

    InputHandler *getInputHandler() const
    {
        return m_input;
    }

    ClientPacketHandler *getPacketHandler() const
    {
        return m_packet_handler.get();
    }
    ClientEventHandler *getClientEventHandler() const
    {
        return m_clientevent_handler.get();
    }

    ChatMessanger *getChatMessanger() const
    {
        return m_chat_msger.get();
    }

    u16 getProtoVersion() const
    { return m_proto_ver; }

    void loadMods();

    void deleteAuthData();
    // helper method shared with clientpackethandler
    static AuthMechanism choseAuthMech(const u32 mechs);

    void startAuth(AuthMechanism chosen_auth_mechanism);

    void sendChangePassword(const std::string &oldpassword,
        const std::string &newpassword);

	bool inhibit_inventory_revert = false;

private:
    void initLocalMapSaving(u16 port, const std::string &hostname,
        bool is_local_server);

    // own state
    LocalClientState m_state = LC_Created;

    std::unique_ptr<MtEventManager> m_eventmgr;
    std::unique_ptr<IWritableItemDefManager> m_itemdef;
    std::unique_ptr<NodeDefManager> m_nodedef;
    std::unique_ptr<ISoundManager> m_sound;
    std::unique_ptr<SoundMaker> m_soundmaker;

    ResourceCache *m_resource_cache;
    RenderSystem *m_render_system;
    InputHandler *m_input;

    std::unique_ptr<ClientPacketHandler> m_packet_handler;
    std::unique_ptr<ClientEventHandler> m_clientevent_handler;
    std::unique_ptr<GameInputSystem> m_inputsys;

	ClientEnvironment m_env;

	Inventory *m_inventory_from_server = nullptr;
	float m_inventory_from_server_age = 0.0f;

	//int m_crack_level = -1;
	//v3s16 m_crack_pos;
	// 0 <= m_daynight_i < DAYNIGHT_CACHE_COUNT
	//s32 m_daynight_i;
	//u32 m_daynight_ratio;
    std::unique_ptr<ChatMessanger> m_chat_msger;

    float m_avg_rtt_timer = 0.0f;

    bool m_mods_loaded = false;

	// The authentication methods we can use to enter sudo mode (=change password)
	u32 m_sudo_auth_methods;

	// The seed returned by the server in TOCLIENT_INIT is stored here
	u64 m_map_seed = 0;

    // Used version of the protocol with server
    // Values smaller than 25 only mean they are smaller than 25,
    // and aren't accurate. We simply just don't know, because
    // the server didn't send the version back then.
    // If 0, server init hasn't been received yet.
    u16 m_proto_ver = 0;

	// Auth data
	std::string m_playername;
	std::string m_password;
	// If set, this will be sent (and cleared) upon a TOCLIENT_ACCEPT_SUDO_MODE
    std::string m_new_password;
    // Usable by auth mechanisms.
    AuthMechanism m_chosen_auth_mech;
    void *m_auth_data = nullptr;

	std::vector<std::string> m_remote_media_servers;
	// Media downloader, only exists during init
	ClientMediaDownloader *m_media_downloader;
	// Pending downloads of dynamic media (key: token)
	std::vector<std::pair<u32, std::shared_ptr<SingleMediaDownloader>>> m_pending_media_downloads;

	// Sounds
	float m_removed_sounds_check_timer = 0.0f;
	// Mapping from server sound ids to our sound ids
	std::unordered_map<s32, sound_handle_t> m_sounds_server_to_client;
	// And the other way!
	// This takes ownership for the sound handles.
	std::unordered_map<sound_handle_t, s32> m_sounds_client_to_server;
	// Relation of client id to object id
	std::unordered_map<sound_handle_t, u16> m_sounds_to_objects;

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

    friend ClientPacketHandler;
};
