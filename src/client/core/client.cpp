// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#include <iostream>
#include <algorithm>
#include <sstream>
#include <cmath>
#include <json/json.h>
#include "client.h"
#include "client/sound/soundopenal.h"
#include "client/ui/gameui.h"
#include "client/ui/hud.h"
#include "client/core/clientevent.h"
#include "client/render/rendersystem.h"
#include "client/sound/sound.h"
#include "client/media/resource.h"
#include "client/player/localplayer.h"
#include "util/auth.h"
#include "util/string.h"
#include "util/srp.h"
#include "filesys.h"
#include "client/map/mapblockmesh.h"
#include "mapblock.h"
#include "mapsector.h"
#include "client/ui/minimap.h"
#include "modchannels.h"
#include "content/mods.h"
#include "profiler.h"
#include "gettext.h"
#include "client/map/clientmap.h"
#include "client/media/clientmedia.h"
#include "database/database-files.h"
#include "script/scripting_client.h"
#include "chatmessage.h"
#include "translation.h"
#include "content/mod_configuration.h"
#include "mapnode.h"
#include "client/network/packethandler.h"
#include "client/ao/clientActiveObject.h"
#include <Image/ImageLoader.h>
#include "client/render/drawlist.h"
#include "client/render/loadscreen.h"
#include "nodedef.h"
#include "database/database-sqlite3.h"
#include "client/core/clienteventhandler.h"
#include "chatmessanger.h"
#include "itemdef.cpp"
#include "client/event/eventmanager.h"
#include "util/quicktune_shortcutter.h"

/*
	Client
*/

Client::Client(ResourceCache *resource_cache, RenderSystem *render_system,
        const char *playername,
        const std::string &password):
    m_eventmgr(std::make_unique<EventManager>()),
    m_itemdef(std::make_unique<CItemDefManager>()),
    m_nodedef(std::make_unique<NodeDefManager>()),
    m_quicktune(std::make_unique<QuicktuneShortcutter>()),
    m_resource_cache(resource_cache),
    m_render_system(render_system),
	m_env(this),
	m_password(password),
	m_chosen_auth_mech(AUTH_MECHANISM_NONE),
	m_media_downloader(new ClientMediaDownloader()),
    m_modchannel_mgr(new ModChannelMgr()),
    m_chat_msger(std::make_unique<ChatMessanger>(this))
{
	// Add local player
	m_env.setLocalPlayer(new LocalPlayer(this, playername));

	// Make the mod storage database and begin the save for later
	m_mod_storage_database =
			new ModStorageDatabaseSQLite3(porting::path_user + DIR_DELIM + "client");
	m_mod_storage_database->beginSave();

	m_cache_save_interval = g_settings->getU16("server_map_save_interval");

    initSound();
}

bool Client::initSound()
{
#if USE_SOUND
    if (g_sound_manager_singleton.get()) {
        infostream << "Attempting to use OpenAL audio" << std::endl;
        m_sound = createOpenALSoundManager(g_sound_manager_singleton.get(),
                std::make_unique<SoundFallbackPathProvider>());
        if (!m_sound)
            infostream << "Failed to initialize OpenAL audio" << std::endl;
    } else {
        infostream << "Sound disabled." << std::endl;
    }
#endif

    if (!m_sound) {
        infostream << "Using dummy audio." << std::endl;
        m_sound = std::make_unique<DummySoundManager>();
    }

    m_soundmaker = std::make_unique<SoundMaker>(m_sound.get(), m_nodedef.get());
    if (!m_soundmaker)
        return false;

    m_soundmaker->registerReceiver(m_eventmgr.get());

    return true;
}

bool Client::shouldShowTouchControls()
{
    const std::string &touch_controls = g_settings->get("touch_controls");
    if (touch_controls == "auto")
        return RenderingEngine::getLastPointerType() == PointerType::Touch;
    return is_yes(touch_controls);
}

bool Client::initGui()
{
    m_render_system->getGameUI()->init();

    m_chat_msger->init(m_render_system->getGUIEnvironment());

    if (shouldShowTouchControls()) {
        g_touchcontrols = new TouchControls(device, texture_src);
        g_touchcontrols->setUseCrosshair(!isTouchCrosshairDisabled());
    }

    return true;
}

void Client::migrateModStorage()
{
	std::string mod_storage_dir = porting::path_user + DIR_DELIM + "client";
	std::string old_mod_storage = mod_storage_dir + DIR_DELIM + "mod_storage";
    if (mt_fs::IsDir(old_mod_storage)) {
		infostream << "Migrating client mod storage to SQLite3 database" << std::endl;
		{
			ModStorageDatabaseFiles files_db(mod_storage_dir);
			std::vector<std::string> mod_list;
			files_db.listMods(&mod_list);
			for (const std::string &modname : mod_list) {
				infostream << "Migrating client mod storage for mod " << modname << std::endl;
				StringMap meta;
				files_db.getModEntries(modname, &meta);
				for (const auto &pair : meta) {
					m_mod_storage_database->setModEntry(modname, pair.first, pair.second);
				}
			}
		}
        if (!mt_fs::Rename(old_mod_storage, old_mod_storage + ".bak")) {
			// Execution cannot move forward if the migration does not complete.
			throw BaseException("Could not finish migrating client mod storage");
		}
		infostream << "Finished migration of client mod storage" << std::endl;
	}
}

void Client::loadMods()
{
	// Don't load mods twice.
	// If client scripting is disabled by the client, don't load builtin or
	// client-provided mods.
	if (m_mods_loaded || !g_settings->getBool("enable_client_modding"))
		return;

	// If client scripting is disabled by the server, don't load builtin or
	// client-provided mods.
	// TODO Delete this code block when server-sent CSM and verifying of builtin are
	// complete.
	if (checkCSMRestrictionFlag(CSMRestrictionFlags::CSM_RF_LOAD_CLIENT_MODS)) {
		warningstream << "Client-provided mod loading is disabled by server." <<
			std::endl;
		return;
	}

	m_script = new ClientScripting(this);
	m_env.setScript(m_script);
	m_script->setEnv(&m_env);

	// Load builtin
	scanModIntoMemory(BUILTIN_MOD_NAME, getBuiltinLuaPath());
	m_script->loadModFromMemory(BUILTIN_MOD_NAME);
	m_script->checkSetByBuiltin();

	ModConfiguration modconf;
	{
		std::unordered_map<std::string, std::string> paths;
		std::string path_user = porting::path_user + DIR_DELIM + "clientmods";
		const auto modsPath = getClientModsLuaPath();
		if (modsPath != path_user) {
			paths["share"] = modsPath;
		}
		paths["mods"] = path_user;

		std::string settings_path = path_user + DIR_DELIM + "mods.conf";
		modconf.addModsFromConfig(settings_path, paths);
		modconf.checkConflictsAndDeps();
	}

	m_mods = modconf.getMods();

	// complain about mods with unsatisfied dependencies
	if (!modconf.isConsistent()) {
		errorstream << modconf.getUnsatisfiedModsError() << std::endl;
		return;
	}

	// Print mods
	infostream << "Client loading mods: ";
	for (const ModSpec &mod : m_mods)
		infostream << mod.name << " ";
	infostream << std::endl;

	// Load "mod" scripts
	for (const ModSpec &mod : m_mods) {
		mod.checkAndLog();
		scanModIntoMemory(mod.name, mod.path);
	}

	// Run them
	for (const ModSpec &mod : m_mods)
		m_script->loadModFromMemory(mod.name);

	// Mods are done loading. Unlock callbacks
	m_mods_loaded = true;

	// Run a callback when mods are loaded
	m_script->on_mods_loaded();

	// Create objects if they're ready
	if (m_state == LC_Ready)
		m_script->on_client_ready(m_env.getLocalPlayer());
    if (m_env.getLocalPlayer()->getCamera())
        m_script->on_camera_ready(m_env.getLocalPlayer()->getCamera());
    if (m_render_system->getGameUI()->getHud()->getMinimap())
        m_script->on_minimap_ready(m_render_system->getGameUI()->getHud()->getMinimap());
}

void Client::scanModSubfolder(const std::string &mod_name, const std::string &mod_path,
			std::string mod_subpath)
{
	std::string full_path = mod_path + DIR_DELIM + mod_subpath;
    std::vector<mt_fs::DirListNode> mod = mt_fs::GetDirListing(full_path);
    for (const mt_fs::DirListNode &j : mod) {
		if (j.name[0] == '.')
			continue;

		if (j.dir) {
			scanModSubfolder(mod_name, mod_path, mod_subpath + j.name + DIR_DELIM);
			continue;
		}
		std::replace(mod_subpath.begin(), mod_subpath.end(), DIR_DELIM_CHAR, '/');

		std::string real_path = full_path + j.name;
		std::string vfs_path = mod_name + ":" + mod_subpath + j.name;
		infostream << "Client::scanModSubfolder(): Loading \"" << real_path
				<< "\" as \"" << vfs_path << "\"." << std::endl;

		std::string contents;
        if (!mt_fs::ReadFile(real_path, contents, true)) {
			continue;
		}

		m_mod_vfs.emplace(vfs_path, contents);
	}
}

const std::string &Client::getBuiltinLuaPath()
{
	static const std::string builtin_dir = porting::path_share + DIR_DELIM + "builtin";
	return builtin_dir;
}

const std::string &Client::getClientModsLuaPath()
{
	static const std::string clientmods_dir = porting::path_share + DIR_DELIM + "clientmods";
	return clientmods_dir;
}

const std::vector<ModSpec>& Client::getMods() const
{
	static std::vector<ModSpec> client_modspec_temp;
	return client_modspec_temp;
}

const ModSpec* Client::getModSpec(const std::string &modname) const
{
	return NULL;
}

void Client::Stop()
{
	m_shutdown = true;
	if (m_mods_loaded)
		m_script->on_shutdown();
	//request all client managed threads to stop
    m_env.getClientMap().stopMeshUpdate();
	// Save local server map
	if (m_localdb) {
		infostream << "Local map saving ended." << std::endl;
		m_localdb->endSave();
	}

	if (m_mods_loaded)
		delete m_script;
}

bool Client::isShutdown()
{
    return m_shutdown || !m_env.getClientMap().isMeshUpdateRunning();
}

Client::~Client()
{
	m_shutdown = true;

	deleteAuthData();

	delete m_inventory_from_server;

	delete m_media_downloader;

	// Write the changes and delete
	if (m_mod_storage_database)
		m_mod_storage_database->endSave();
	delete m_mod_storage_database;

	// Free sound ids
	for (auto &csp : m_sounds_client_to_server)
		m_sound->freeId(csp.first);
	m_sounds_client_to_server.clear();
}

void Client::step(float dtime)
{
	// Limit a bit
	if (dtime > DTIME_LIMIT)
		dtime = DTIME_LIMIT;

    m_packet_handler->receiveAll();

	/*
		Packet counter
	*/
    m_packet_handler->printPacketCounter(dtime);

	// The issue that made this workaround necessary was fixed in August 2024, but
	// it's not like we can remove this code - ever.
    if (m_state == LC_Created) {
        m_packet_handler->sendInit(dtime);

		// Not connected, return
		return;
    }

	/*
		Do stuff if connected
	*/

	/*
		Send pending messages on out chat queue
	*/
    m_chat_msger->sendFromQueue();

	/*
		Handle environment
	*/
	LocalPlayer *player = m_env.getLocalPlayer();

	// Step environment (also handles player controls)
	m_env.step(dtime);
	m_sound->step(dtime);

	/*
		Get events
	*/
	while (m_env.hasClientEnvEvents()) {
		ClientEnvEvent envEvent = m_env.getClientEnvEvent();

		if (envEvent.type == CEE_PLAYER_DAMAGE) {
			u16 damage = envEvent.player_damage.amount;

			if (envEvent.player_damage.send_to_server)
                m_packet_handler->sendDamage(damage);

			// Add to ClientEvent queue
			ClientEvent *event = new ClientEvent();
			event->type = CE_PLAYER_DAMAGE;
			event->player_damage.amount = damage;
			event->player_damage.effect = true;
            m_clientevent_handler->pushToEventQueue(event);
		}
	}

	/*
		Print some info
	*/
	float &counter = m_avg_rtt_timer;
	counter += dtime;
	if(counter >= 10) {
		counter = 0.0;
		// connectedAndInitialized() is true, peer exists.
        float avg_rtt = m_packet_handler->getRTT();
		infostream << "Client: avg_rtt=" << avg_rtt << std::endl;
	}

	/*
		Send player position to server
	*/
    m_packet_handler->sendPlayerPos(m_render_system->getDrawList()->getDrawControl().wanted_range, dtime);

	/*
		Load fetched media
	*/
	if (m_media_downloader && m_media_downloader->isStarted()) {
		m_media_downloader->step(this);
		if (m_media_downloader->isDone()) {
			delete m_media_downloader;
			m_media_downloader = NULL;
		}
	}
	{
		// Acknowledge dynamic media downloads to server
		std::vector<u32> done;
		for (auto it = m_pending_media_downloads.begin();
				it != m_pending_media_downloads.end();) {
			assert(it->second->isStarted());
			it->second->step(this);
			if (it->second->isDone()) {
				done.emplace_back(it->first);

				it = m_pending_media_downloads.erase(it);
			} else {
				it++;
			}

			if (done.size() == 255) { // maximum in one packet
                m_packet_handler->sendHaveMedia(done);
				done.clear();
			}
		}
		if (!done.empty())
            m_packet_handler->sendHaveMedia(done);
	}

	/*
		If the server didn't update the inventory in a while, revert
		the local inventory (so the player notices the lag problem
		and knows something is wrong).
	*/
	if (m_inventory_from_server && !inhibit_inventory_revert) {
		float interval = 10.0f;
		float count_before = std::floor(m_inventory_from_server_age / interval);

		m_inventory_from_server_age += dtime;

		float count_after = std::floor(m_inventory_from_server_age / interval);

		if (count_after != count_before) {
			// Do this every <interval> seconds after TOCLIENT_INVENTORY
			// Reset the locally changed inventory to the authoritative inventory
			player->inventory = *m_inventory_from_server;
            player->updateWieldedItem();
		}
	}

	/*
		Update positions of sounds attached to objects
	*/
	{
		for (auto &m_sounds_to_object : m_sounds_to_objects) {
			sound_handle_t client_id = m_sounds_to_object.first;
			u16 object_id = m_sounds_to_object.second;
			ClientActiveObject *cao = m_env.getActiveObject(object_id);
			if (!cao)
				continue;
			m_sound->updateSoundPosVel(client_id, cao->getPosition() * (1.0f/BS),
					cao->getVelocity() * (1.0f/BS));
		}
	}

	/*
		Handle removed remotely initiated sounds
	*/
	m_removed_sounds_check_timer += dtime;
	if(m_removed_sounds_check_timer >= 2.32) {
		m_removed_sounds_check_timer = 0;
		// Find removed sounds and clear references to them
		std::vector<sound_handle_t> removed_client_ids = m_sound->pollRemovedSounds();
		std::vector<s32> removed_server_ids;
		for (sound_handle_t client_id : removed_client_ids) {
			auto client_to_server_id_it = m_sounds_client_to_server.find(client_id);
			if (client_to_server_id_it == m_sounds_client_to_server.end())
				continue;
			s32 server_id = client_to_server_id_it->second;
			m_sound->freeId(client_id);
			m_sounds_client_to_server.erase(client_to_server_id_it);
			if (server_id != -1) {
				m_sounds_server_to_client.erase(server_id);
				removed_server_ids.push_back(server_id);
			}
			m_sounds_to_objects.erase(client_id);
		}

		// Sync to server
		if (!removed_server_ids.empty()) {
            m_packet_handler->sendRemovedSounds(removed_server_ids);
		}
	}

	// Write changes to the mod storage
	m_mod_storage_save_timer -= dtime;
	if (m_mod_storage_save_timer <= 0.0f) {
		m_mod_storage_save_timer = g_settings->getFloat("server_map_save_interval");
		m_mod_storage_database->endSave();
		m_mod_storage_database->beginSave();
	}

	// Write server map
	if (m_localdb && m_localdb_save_interval.step(dtime,
			m_cache_save_interval)) {
		m_localdb->endSave();
		m_localdb->beginSave();
	}
}

bool Client::loadMedia(const std::string &data, const std::string &filename,
	bool from_media_push)
{
	std::string name;

	const char *image_ext[] = {
		".png", ".jpg", ".tga",
		NULL
	};
	name = removeStringEnd(filename, image_ext);
	if (!name.empty()) {
		TRACESTREAM(<< "Client: Attempting to load image "
			<< "file \"" << filename << "\"" << std::endl);

		// Read image
        auto *img = img::ImageLoader::loadFromMem((void *)data.c_str(), data.size());
		if (!img) {
			errorstream<<"Client: Cannot create image from data of "
					<<"file \""<<filename<<"\""<<std::endl;
            delete img;
			return false;
		}

        m_resource_cache->cacheResource<img::Image>(ResourceType::IMAGE, img, filename);

		return true;
	}

	const char *sound_ext[] = {
		".0.ogg", ".1.ogg", ".2.ogg", ".3.ogg", ".4.ogg",
		".5.ogg", ".6.ogg", ".7.ogg", ".8.ogg", ".9.ogg",
		".ogg", NULL
	};
	name = removeStringEnd(filename, sound_ext);
	if (!name.empty()) {
		TRACESTREAM(<< "Client: Attempting to load sound file \""
				<< filename << "\"" << std::endl);
		if (!m_sound->loadSoundData(filename, std::string(data)))
			return false;
		// "name[.num].ogg" is in group "name"
		m_sound->addSoundToGroup(filename, name);
		return true;
	}

	const char *model_ext[] = {
		".x", ".b3d", ".obj", ".gltf", ".glb",
		NULL
	};
	name = removeStringEnd(filename, model_ext);
	if (!name.empty()) {
		verbosestream<<"Client: Storing model into memory: "
				<<"\""<<filename<<"\""<<std::endl;
		if(m_mesh_data.count(filename))
			errorstream<<"Multiple models with name \""<<filename.c_str()
					<<"\" found; replacing previous model"<<std::endl;
		m_mesh_data[filename] = data;
		return true;
	}

	if (Translations::isTranslationFile(filename)) {
		if (from_media_push)
			return false;
		TRACESTREAM(<< "Client: Loading translation: "
				<< "\"" << filename << "\"" << std::endl);
		g_client_translations->loadTranslation(filename, data);
		return true;
	}

	errorstream << "Client: Don't know how to load file \""
		<< filename << "\"" << std::endl;
	return false;
}

/*
	u16 command
	u16 number of files requested
	for each file {
		u16 length of name
		string name
	}
*/

void Client::initLocalMapSaving(u16 port,
		const std::string &hostname,
		bool is_local_server)
{
	if (!g_settings->getBool("enable_local_map_saving") || is_local_server) {
		return;
	}
	if (m_localdb) {
		infostream << "Local map saving already running" << std::endl;
		return;
	}

	std::string world_path;
#define set_world_path(hostname) \
	world_path = porting::path_user \
		+ DIR_DELIM + "worlds" \
		+ DIR_DELIM + "server_" \
        + hostname + "_" + std::to_string(port);

	set_world_path(hostname);
    if (!mt_fs::IsDir(world_path)) {
		std::string hostname_escaped = hostname;
		str_replace(hostname_escaped, ':', '_');
		set_world_path(hostname_escaped);
	}
#undef set_world_path
    mt_fs::CreateAllDirs(world_path);

	m_localdb = new MapDatabaseSQLite3(world_path);
	m_localdb->beginSave();
	actionstream << "Local map saving started, map will be saved at '" << world_path << "'" << std::endl;
}

void Client::deleteAuthData()
{
	if (!m_auth_data)
		return;

	switch (m_chosen_auth_mech) {
		case AUTH_MECHANISM_FIRST_SRP:
			break;
		case AUTH_MECHANISM_SRP:
		case AUTH_MECHANISM_LEGACY_PASSWORD:
			srp_user_delete((SRPUser *) m_auth_data);
			m_auth_data = NULL;
			break;
		case AUTH_MECHANISM_NONE:
			break;
	}
	m_chosen_auth_mech = AUTH_MECHANISM_NONE;
}


AuthMechanism Client::choseAuthMech(const u32 mechs)
{
	if (mechs & AUTH_MECHANISM_SRP)
		return AUTH_MECHANISM_SRP;

	if (mechs & AUTH_MECHANISM_FIRST_SRP)
		return AUTH_MECHANISM_FIRST_SRP;

	if (mechs & AUTH_MECHANISM_LEGACY_PASSWORD)
		return AUTH_MECHANISM_LEGACY_PASSWORD;

	return AUTH_MECHANISM_NONE;
}

void Client::startAuth(AuthMechanism chosen_auth_mechanism)
{
	m_chosen_auth_mech = chosen_auth_mechanism;

	std::string playername = m_env.getLocalPlayer()->getName();

	switch (chosen_auth_mechanism) {
		case AUTH_MECHANISM_FIRST_SRP: {
			// send srp verifier to server
			std::string verifier;
			std::string salt;
			generate_srp_verifier_and_salt(playername, m_password,
				&verifier, &salt);

            m_packet_handler->sendAuthFirstSRP(verifier, salt, m_password.empty());
			break;
		}
		case AUTH_MECHANISM_SRP:
		case AUTH_MECHANISM_LEGACY_PASSWORD: {
			u8 based_on = 1;

			if (chosen_auth_mechanism == AUTH_MECHANISM_LEGACY_PASSWORD) {
				m_password = translate_password(playername, m_password);
				based_on = 0;
			}

			std::string playername_u = lowercase(playername);
			m_auth_data = srp_user_new(SRP_SHA256, SRP_NG_2048,
				playername.c_str(), playername_u.c_str(),
				(const unsigned char *) m_password.c_str(),
				m_password.length(), NULL, NULL);
			char *bytes_A = 0;
			size_t len_A = 0;
			SRP_Result res = srp_user_start_authentication(
				(struct SRPUser *) m_auth_data, NULL, NULL, 0,
				(unsigned char **) &bytes_A, &len_A);
			FATAL_ERROR_IF(res != SRP_OK, "Creating local SRP user failed.");

            m_packet_handler->sendAuthSRPBytesA(bytes_A, len_A, based_on);
			break;
		}
		case AUTH_MECHANISM_NONE:
			break; // not handled in this method
	}
}

void Client::sendChangePassword(const std::string &oldpassword,
	const std::string &newpassword)
{
	LocalPlayer *player = m_env.getLocalPlayer();
	if (player == NULL)
		return;

	// get into sudo mode and then send new password to server
	m_password = oldpassword;
	m_new_password = newpassword;
	startAuth(choseAuthMech(m_sudo_auth_methods));
}

/**
 * Helper function for Client Side Modding
 * CSM restrictions are applied there, this should not be used for core engine
 * @param p
 * @param is_valid_position
 * @return
 */
MapNode Client::CSMGetNode(v3s16 p, bool *is_valid_position)
{
	if (checkCSMRestrictionFlag(CSMRestrictionFlags::CSM_RF_LOOKUP_NODES)) {
		v3s16 ppos = floatToInt(m_env.getLocalPlayer()->getPosition(), BS);
		if ((u32) ppos.getDistanceFrom(p) > m_csm_restriction_noderange) {
			*is_valid_position = false;
			return {};
		}
	}
	return m_env.getMap().getNode(p, is_valid_position);
}

int Client::CSMClampRadius(v3s16 pos, int radius)
{
	if (!checkCSMRestrictionFlag(CSMRestrictionFlags::CSM_RF_LOOKUP_NODES))
		return radius;
	// This is approximate and will cause some allowed nodes to be excluded
	v3s16 ppos = floatToInt(m_env.getLocalPlayer()->getPosition(), BS);
	u32 distance = ppos.getDistanceFrom(pos);
	if (distance >= m_csm_restriction_noderange)
		return 0;
	return std::min<int>(radius, m_csm_restriction_noderange - distance);
}

v3s16 Client::CSMClampPos(v3s16 pos)
{
	if (!checkCSMRestrictionFlag(CSMRestrictionFlags::CSM_RF_LOOKUP_NODES))
		return pos;
	v3s16 ppos = floatToInt(m_env.getLocalPlayer()->getPosition(), BS);
	const int range = m_csm_restriction_noderange;
	return v3s16(
        std::clamp<int>(pos.X, (int)ppos.X - range, (int)ppos.X + range),
        std::clamp<int>(pos.Y, (int)ppos.Y - range, (int)ppos.Y + range),
        std::clamp<int>(pos.Z, (int)ppos.Z - range, (int)ppos.Z + range)
	);
}

/*int Client::getCrackLevel()
{
	return m_crack_level;
}

v3s16 Client::getCrackPos()
{
	return m_crack_pos;
}

void Client::setCrack(int level, v3s16 pos)
{
	int old_crack_level = m_crack_level;
	v3s16 old_crack_pos = m_crack_pos;

	m_crack_level = level;
	m_crack_pos = pos;

	if(old_crack_level >= 0 && (level < 0 || pos != old_crack_pos))
	{
		// remove old crack
		addUpdateMeshTaskForNode(old_crack_pos, false, true);
	}
	if(level >= 0 && (old_crack_level < 0 || pos != old_crack_pos))
	{
		// add new crack
		addUpdateMeshTaskForNode(pos, false, true);
	}
}*/

float Client::mediaReceiveProgress()
{
	if (m_media_downloader)
		return m_media_downloader->getProgress();

	return 1.0; // downloader only exists when not yet done
}

/*void Client::drawLoadScreen(const std::wstring &text, float dtime, int percent) {
	m_rendering_engine->run();
	m_rendering_engine->draw_load_screen(text, guienv, m_tsrc, dtime, percent);
}*/

struct TextureUpdateArgs {
    gui::IGUIEnvironment *guienv;
	u64 last_time_ms;
	u16 last_percent;
	std::wstring text_base;
    ResourceCache *cache;
};

void Client::showUpdateProgressTexture(void *args, u32 progress, u32 max_progress)
{
		TextureUpdateArgs* targs = (TextureUpdateArgs*) args;
		u16 cur_percent = std::ceil(progress / max_progress * 100.f);

		// update the loading menu -- if necessary
		bool do_draw = false;
		u64 time_ms = targs->last_time_ms;
		if (cur_percent != targs->last_percent) {
			targs->last_percent = cur_percent;
			time_ms = porting::getTimeMs();
			// only draw when the user will notice something:
			do_draw = (time_ms - targs->last_time_ms > 100);
		}

		if (do_draw) {
			targs->last_time_ms = time_ms;
			std::wostringstream strm;
			strm << targs->text_base << L" " << targs->last_percent << L"%...";
            v2u wndsize = m_render_system->getWindowSize();
            m_render_system->getLoadScreen()->draw(wndsize, strm.str(), 0.0f, g_settings->getBool("menu_clouds"),
                72 + (u16) ((18. / 100.) * (double) targs->last_percent), m_render_system->getScaleFactor());
		}
}

void Client::afterContentReceived()
{
	infostream<<"Client::afterContentReceived() started"<<std::endl;
    assert(m_packet_handler->itemdefReceived()); // pre-condition
    assert(m_packet_handler->nodedefReceived()); // pre-condition

    auto loadscreen = m_render_system->getLoadScreen();
    v2u wndsize = m_render_system->getWindowSize();
    bool enable_clouds = g_settings->getBool("menu_clouds");
    f32 scale_factor = m_render_system->getScaleFactor();

	// Update node aliases
	infostream<<"- Updating node aliases"<<std::endl;
    loadscreen->draw(wndsize, wstrgettext("Initializing nodes..."), 0.0f, enable_clouds, 72, scale_factor);
    m_nodedef->updateAliases(m_itemdef.get());

    fs::path base_pack;
    base_pack /= fs::path("base") /= "pack";
    for (const auto &path : getTexturesDefaultPaths()) {
        if (!str_ends_with(path, base_pack.string())) {
            TextureOverrideSource override_source(path + DIR_DELIM + "override.txt");
            m_nodedef->applyTextureOverrides(override_source.getNodeTileOverrides());
            m_itemdef->applyTextureOverrides(override_source.getItemTextureOverrides());
        }
	}
	m_nodedef->setNodeRegistrationStatus(true);
	m_nodedef->runNodeResolveCallbacks();

	// Update node textures and assign shaders to each tile
	infostream<<"- Updating node textures"<<std::endl;
	TextureUpdateArgs tu_args;
    tu_args.guienv = m_render_system->getGUIEnvironment();
	tu_args.last_time_ms = porting::getTimeMs();
	tu_args.last_percent = 0;
	tu_args.text_base = wstrgettext("Initializing nodes");
    tu_args.cache = m_resource_cache;
	m_nodedef->updateTextures(this, &tu_args);

	// Start mesh update thread after setting up content definitions
	infostream<<"- Starting mesh update thread"<<std::endl;
    m_env.getClientMap().startMeshUpdate();

	m_state = LC_Ready;
    m_packet_handler->sendReady();

	if (m_mods_loaded)
		m_script->on_client_ready(m_env.getLocalPlayer());

    wndsize = m_render_system->getWindowSize();
    loadscreen->draw(wndsize, wstrgettext("Done!"), 0.0f, enable_clouds, 100, scale_factor);
	infostream<<"Client::afterContentReceived() done"<<std::endl;
}

/*void Client::makeScreenshot()
{
	irr::video::IVideoDriver *driver = m_rendering_engine->get_video_driver();
	irr::video::IImage* const raw_image = driver->createScreenShot();

	if (!raw_image)
		return;

	const struct tm tm = mt_localtime();

	char timetstamp_c[64];
	strftime(timetstamp_c, sizeof(timetstamp_c), "%Y%m%d_%H%M%S", &tm);

	std::string screenshot_dir;

	if (fs::IsPathAbsolute(g_settings->get("screenshot_path")))
		screenshot_dir = g_settings->get("screenshot_path");
	else
		screenshot_dir = porting::path_user + DIR_DELIM + g_settings->get("screenshot_path");

	std::string filename_base = screenshot_dir
			+ DIR_DELIM
			+ std::string("screenshot_")
			+ std::string(timetstamp_c);
	std::string filename_ext = "." + g_settings->get("screenshot_format");
	std::string filename;

	// Create the directory if it doesn't already exist.
	// Otherwise, saving the screenshot would fail.
	fs::CreateDir(screenshot_dir);

	u32 quality = (u32)g_settings->getS32("screenshot_quality");
	quality = MYMIN(MYMAX(quality, 0), 100) / 100.0 * 255;

	// Try to find a unique filename
	unsigned serial = 0;

	while (serial < SCREENSHOT_MAX_SERIAL_TRIES) {
		filename = filename_base + (serial > 0 ? ("_" + itos(serial)) : "") + filename_ext;
		if (!fs::PathExists(filename))
			break;	// File did not apparently exist, we'll go with it
		serial++;
	}

	if (serial == SCREENSHOT_MAX_SERIAL_TRIES) {
		infostream << "Could not find suitable filename for screenshot" << std::endl;
	} else {
		irr::video::IImage* const image =
				driver->createImage(video::ECF_R8G8B8, raw_image->getDimension());

		if (image) {
			raw_image->copyTo(image);

			std::ostringstream sstr;
			if (driver->writeImageToFile(image, filename.c_str(), quality)) {
				sstr << "Saved screenshot to '" << filename << "'";
			} else {
				sstr << "Failed to save screenshot '" << filename << "'";
			}
			pushToChatQueue(new ChatMessage(CHATMESSAGE_TYPE_SYSTEM,
					utf8_to_wide(sstr.str())));
			infostream << sstr.str() << std::endl;
			image->drop();
		}
	}

	raw_image->drop();
}*/

// IGameDef interface
// Under envlock
IItemDefManager* Client::getItemDefManager()
{
    return m_itemdef.get();
}
const NodeDefManager* Client::getNodeDefManager()
{
    return m_nodedef.get();
}
ICraftDefManager* Client::getCraftDefManager()
{
	return NULL;
	//return m_craftdef;
}

u16 Client::allocateUnknownNodeId(const std::string &name)
{
	errorstream << "Client::allocateUnknownNodeId(): "
			<< "Client cannot allocate node IDs" << std::endl;
	FATAL_ERROR("Client allocated unknown node");

	return CONTENT_IGNORE;
}
ISoundManager* Client::getSoundManager()
{
    return m_sound.get();
}
MtEventManager* Client::getEventManager()
{
	return m_event;
}

const std::string* Client::getModFile(std::string filename)
{
	// strip dir delimiter from beginning of path
	auto pos = filename.find_first_of(':');
	if (pos == std::string::npos)
		return nullptr;
	pos++;
	auto pos2 = filename.find_first_not_of('/', pos);
	if (pos2 > pos)
		filename.erase(pos, pos2 - pos);

	StringMap::const_iterator it = m_mod_vfs.find(filename);
	if (it == m_mod_vfs.end())
		return nullptr;
	return &it->second;
}

ModChannel* Client::getModChannel(const std::string &channel)
{
	return m_modchannel_mgr->getModChannel(channel);
}

const std::string &Client::getFormspecPrepend() const
{
	return m_env.getLocalPlayer()->formspec_prepend;
}
