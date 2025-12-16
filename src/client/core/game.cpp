// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#include "game.h"

#include "client.h"
#include <cmath>
#include "client/player/playercamera.h"
#include "client/render/renderer.h"
#include "clientevent.h"
#include "client/ui/gameui.h"
#include "client/ui/gameformspec.h"
#include "client/event/inputhandler.h"
#include "client/event/keytype.h"
#include "client/event/joystickcontroller.h"
#include "client/map/mapblockmesh.h"
#include "client/sound/sound.h"
#include "client/map/clientmap.h"
#include "client/media/clientmedia.h" // For clientMediaUpdateCacheCopy
#include "client/render/clouds.h"
#include "config.h"
#include "client/ao/genericCAO.h"
#include "content/subgames.h"
#include "client/event/eventmanager.h"
#include "client/ui/glyph_atlas.h"
#include "gui/touchcontrols.h"
#include "itemdef.h"
#include "log.h"
#include "log_internal.h"
#include "gameparams.h"
#include "gettext.h"
#include "gui/guiChatConsole.h"
#include "gui/mainmenumanager.h"
#include "client/ui/profilergraph.h"
#include "client/ui/minimap.h"
#include "nodedef.h"         // Needed for determining pointing to nodes
#include "nodemetadata.h"
#include "client/render/particles.h"
#include "porting.h"
#include "profiler.h"
#include "raycast.h"
#include "server.h"
#include "settings.h"
#include "client/render/sky.h"
#include "threading/lambda.h"
#include "translation.h"
#include "util/basic_macros.h"
#include "util/directiontables.h"
#include "util/pointedthing.h"
#include "util/quicktune_shortcutter.h"
#include "version.h"
#include "script/scripting_client.h"
#include "client/ui/hud.h"
#include "clientdynamicinfo.h"
#include "util/tracy_wrapper.h"
#include "client/sound/soundmaker.h"
#include "client/render/rendersystem.h"
#include "client/network/packethandler.h"
#include "client/render/loadscreen.h"

#if USE_SOUND
    #include "client/sound/soundopenal.h"
#endif

/****************************************************************************
 ****************************************************************************/

Game::Game()
{
	g_settings->registerChangedCallback("pause_on_lost_focus",
		&settingChangedCallback, this);

	readSettings();
}



/****************************************************************************
 MinetestApp Public
 ****************************************************************************/

Game::~Game()
{
	g_settings->deregisterAllChangedCallbacks(this);
}

bool Game::startup(bool *kill, InputHandler *input, RenderSystem *rndsys, ResourceCache *rescache,
        const GameStartData &start_data,
        std::string &error_message,
        bool *reconnect)
{
	// "cache"
	this->kill                = kill;
	this->error_message       = &error_message;
	reconnect_requested       = reconnect;
	simple_singleplayer_mode  = start_data.isSinglePlayer();

	input->keycache.populate();

    this->rndsys = rndsys;
    this->rescache = rescache;
    this->input = input;

    g_client_translations->clear();

    rndsys->initLoadScreen();

    showOverlayMessage(N_("Loading..."), 0, 0);

    // Create a server if not connecting to an existing one
    // address can change if simple_singleplayer_mode
    if (start_data.address.empty() && !createSingleplayerServer(
                start_data.world_spec.path,
                start_data.game_spec, start_data.socket_port)) {
            return false;
    }

	if (!createClient(start_data))
		return false;

	return true;
}


void Game::run()
{
	ZoneScoped;

	f32 dtime; // in seconds

	// Clear the profiler
	{
		Profiler::GraphValues dummyvalues;
		g_profiler->graphPop(dummyvalues);
	}

    auto drawstats = rndsys->getRenderer()->getDrawStats();
    drawstats.fps.reset();

	set_light_table(g_settings->getFloat("display_gamma"));

    const v2u initial_screen_size(
			g_settings->getU16("screen_w"),
			g_settings->getU16("screen_h")
		);
	const bool initial_window_maximized = !g_settings->getBool("fullscreen") &&
			g_settings->getBool("window_maximized");

	auto framemarker = FrameMarker("Game::run()-frame").started();

    while (rndsys->run()
			&& !(*kill || g_gamecallback->shutdown_requested
			|| (server && server->isShutdownRequested()))) {

		framemarker.end();

		// Calculate dtime =
		//    m_rendering_engine->run() from this iteration
		//  + Sleep time until the wanted FPS are reached
        drawstats.fps.limit(rndsys->getWindow(), &dtime);

		framemarker.start();

        const auto current_dynamic_info = ClientDynamicInfo::getCurrent(rndsys);
		if (!current_dynamic_info.equal(client_display_info)) {
			client_display_info = current_dynamic_info;
			dynamic_info_send_timer = 0.2f;
		}

        auto pkt_handler = client->getPacketHandler();
		if (dynamic_info_send_timer > 0.0f) {
			dynamic_info_send_timer -= dtime;
			if (dynamic_info_send_timer <= 0.0f) {
                pkt_handler->sendUpdateClientInfo(current_dynamic_info);
			}
		}

		// Prepare render data for next iteration

        rndsys->getRenderer()->updateStats(dtime);

        if (!pkt_handler->checkConnection(error_message, reconnect_requested))
			break;
        if (!rndsys->getGameFormSpec()->handleCallbacks())
			break;

		updatePauseState();
		if (m_is_paused)
			dtime = 0.0f;

		step(dtime);

        client->getEnv().updateFrame(dtime, m_is_paused);

        if (m_does_lost_focus_pause_game && !rndsys->getWindow()->isFocused() && !isMenuActive()) {
            rndsys->getGameFormSpec()->showPauseMenu();
		}
	}

	framemarker.end();

    rndsys->autosaveScreensizeAndCo(initial_screen_size, initial_window_maximized);
}


void Game::shutdown()
{
	if (g_touchcontrols)
		g_touchcontrols->hide();

	// only if the shutdown progress bar isn't shown yet
	if (m_shutdown_progress == 0.0f)
        showOverlayMessage(N_("Shutting down..."), 0, 0);

	/* cleanup menus */
    while (g_menumgr->menuCount() > 0) {
        g_menumgr->deleteFront();
	}

	auto stop_thread = runInThread([=] {
        server.reset();
	}, "ServerStop");

    auto drawstats = rndsys->getRenderer()->getDrawStats();
    drawstats.fps.reset();

	while (stop_thread->isRunning()) {
        rndsys->run();
		f32 dtime;
        drawstats.fps.limit(rndsys->getWindow(), &dtime);
		showOverlayMessage(N_("Shutting down..."), dtime, 0, &m_shutdown_progress);
	}

	stop_thread->rethrow();

	// to be continued in Game::~Game
}


/****************************************************************************/
/****************************************************************************
 Startup
 ****************************************************************************/
/****************************************************************************/

bool Game::createSingleplayerServer(const std::string &map_dir,
		const SubgameSpec &gamespec, u16 port)
{
	showOverlayMessage(N_("Creating server..."), 0, 5);

	std::string bind_str;
	if (simple_singleplayer_mode) {
		// Make the simple singleplayer server only accept connections from localhost,
		// which also makes Windows Defender not show a warning.
		bind_str = "127.0.0.1";
	} else {
		bind_str = g_settings->get("bind_address");
	}

	Address bind_addr(0, 0, 0, 0, port);

	if (g_settings->getBool("ipv6_server"))
		bind_addr.setAddress(static_cast<IPv6AddressBytes*>(nullptr));
	try {
		bind_addr.Resolve(bind_str.c_str());
	} catch (const ResolveError &e) {
		warningstream << "Resolving bind address \"" << bind_str
			<< "\" failed: " << e.what()
			<< " -- Listening on all addresses." << std::endl;
	}
	if (bind_addr.isIPv6() && !g_settings->getBool("enable_ipv6")) {
		*error_message = fmtgettext("Unable to listen on %s because IPv6 is disabled",
			bind_addr.serializeString().c_str());
		errorstream << *error_message << std::endl;
		return false;
	}

    server = std::make_unique<Server>(map_dir, gamespec, simple_singleplayer_mode, bind_addr,
			false, nullptr, error_message);

	auto start_thread = runInThread([=] {
		server->start();
		copyServerClientCache();
	}, "ServerStart");

	input->clear();
	bool success = true;

    auto drawstats = rndsys->getRenderer()->getDrawStats();
    drawstats.fps.reset();

	while (start_thread->isRunning()) {
        if (!rndsys->run() || input->cancelPressed())
			success = false;
		f32 dtime;
        drawstats.fps.limit(rndsys->getWindow(), &dtime);

		if (success)
			showOverlayMessage(N_("Creating server..."), dtime, 5);
		else
			showOverlayMessage(N_("Shutting down..."), dtime, 0, &m_shutdown_progress);
	}

	start_thread->rethrow();

	return success;
}

void Game::copyServerClientCache()
{
	// It would be possible to let the client directly read the media files
	// from where the server knows they are. But aside from being more complicated
	// it would also *not* fill the media cache and cause slower joining of
	// remote servers.
	// (Imagine that you launch a game once locally and then connect to a server.)

	assert(server);
	auto map = server->getMediaList();
	u32 n = 0;
	for (auto &it : map) {
		assert(it.first.size() == 20); // SHA1
		if (clientMediaUpdateCacheCopy(it.first, it.second))
			n++;
	}
	infostream << "Copied " << n << " files directly from server to client cache"
		<< std::endl;
}

bool Game::createClient(const GameStartData &start_data)
{
	showOverlayMessage(N_("Creating client..."), 0, 10);

	bool could_connect, connect_aborted;
	if (!connectToServer(start_data, &could_connect, &connect_aborted))
		return false;

	if (!could_connect) {
		if (error_message->empty() && !connect_aborted) {
			// Should not happen if error messages are set properly
			*error_message = gettext("Connection failed for unknown reason");
			errorstream << *error_message << std::endl;
		}
		return false;
	}

	if (!getServerContent(&connect_aborted)) {
		if (error_message->empty() && !connect_aborted) {
			// Should not happen if error messages are set properly
			*error_message = gettext("Connection failed for unknown reason");
			errorstream << *error_message << std::endl;
		}
		return false;
	}

    //ShadowRenderer::preInit(shader_src)

	/* Pre-calculated values
	 */
    /*video::ITexture *t = texture_src->getTexture("crack_anylength.png");
	if (t) {
		v2u32 size = t->getOriginalSize();
		crack_animation_length = size.Y / size.X;
	} else {
		crack_animation_length = 5;
    }*/

    client->getRenderSystem()->initRenderEnvironment(client.get());

    if (!client->initGui())
        return false;

    client->initInput();

	/* Set window caption
	 */
    auto wnd = rndsys->getWindow();
    auto driver_name = wnd->getGLVersionString();
	std::string str = std::string(PROJECT_NAME_C) +
			" " + g_version_hash + " [";
	str += simple_singleplayer_mode ? gettext("Singleplayer")
			: gettext("Multiplayer");
	str += "] [";
	str += driver_name;
	str += "]";

    wnd->setCaption(utf8_to_wide(str));

    // Update cached textures, meshes and materials
    client->afterContentReceived();

    // Since Sky needs the 3d atlas, init it after collecting the atlas tiles for nodes
    client->getRenderSystem()->initSkybox();

	return true;
}

bool Game::connectToServer(const GameStartData &start_data,
		bool *connect_ok, bool *connection_aborted)
{
	*connect_ok = false;	// Let's not be overly optimistic
	*connection_aborted = false;
	bool local_server_mode = false;
	const auto &address_name = start_data.address;

	showOverlayMessage(N_("Resolving address..."), 0, 15);

	Address connect_address(0, 0, 0, 0, start_data.socket_port);
	Address fallback_address;

	try {
		connect_address.Resolve(address_name.c_str(), &fallback_address);

		if (connect_address.isAny()) {
			// replace with localhost IP
			if (connect_address.isIPv6()) {
				IPv6AddressBytes addr_bytes;
				addr_bytes.bytes[15] = 1;
				connect_address.setAddress(&addr_bytes);
			} else {
				connect_address.setAddress(127, 0, 0, 1);
			}
			local_server_mode = true;
		}
	} catch (ResolveError &e) {
		*error_message = fmtgettext("Couldn't resolve address: %s", e.what());

		errorstream << *error_message << std::endl;
		return false;
	}

	// this shouldn't normally happen since Address::Resolve() checks for enable_ipv6
	if (g_settings->getBool("enable_ipv6")) {
		// empty
	} else if (connect_address.isIPv6()) {
		*error_message = fmtgettext("Unable to connect to %s because IPv6 is disabled", connect_address.serializeString().c_str());
		errorstream << *error_message << std::endl;
		return false;
	} else if (fallback_address.isIPv6()) {
		fallback_address = Address();
	}

	fallback_address.setPort(connect_address.getPort());
	if (fallback_address.isValid()) {
		infostream << "Resolved two addresses for \"" << address_name
			<< "\" isIPv6[0]=" << connect_address.isIPv6()
			<< " isIPv6[1]=" << fallback_address.isIPv6() << std::endl;
	} else {
		infostream << "Resolved one address for \"" << address_name
			<< "\" isIPv6=" << connect_address.isIPv6() << std::endl;
	}


	try {
        client = std::make_unique<Client>(
                rescache,
                rndsys,
                input,
                start_data.name.c_str(),
                start_data.password,
                start_data.allow_login_or_register);
	} catch (const BaseException &e) {
		*error_message = fmtgettext("Error creating client: %s", e.what());
		errorstream << *error_message << std::endl;
		return false;
	}

	client->migrateModStorage();
	client->m_simple_singleplayer_mode = simple_singleplayer_mode;

    if (!client->initSound()) {
        return false;
    }

	/*
		Wait for server to accept connection
	*/

    auto pkt_handler = client->getPacketHandler();

    pkt_handler->connect(connect_address, address_name,
		simple_singleplayer_mode || local_server_mode);

	try {
		input->clear();

		f32 dtime;
		f32 wait_time = 0; // in seconds
		bool did_fallback = false;

        auto drawstats = rndsys->getRenderer()->getDrawStats();
        drawstats.fps.reset();

		auto framemarker = FrameMarker("Game::connectToServer()-frame").started();

        while (rndsys->run()) {

			framemarker.end();
            drawstats.fps.limit(rndsys->getWindow(), &dtime);
			framemarker.start();

			// Update client and server
			step(dtime);

			// End condition
			if (client->getState() == LC_Init) {
				*connect_ok = true;
				break;
			}

			// Break conditions
			if (*connection_aborted)
				break;

            if (pkt_handler->accessDenied()) {
                *error_message = fmtgettext("Access denied. Reason: %s", pkt_handler->accessDeniedReason().c_str());
                *reconnect_requested = pkt_handler->reconnectRequested();
				errorstream << *error_message << std::endl;
				break;
			}

			if (input->cancelPressed()) {
				*connection_aborted = true;
				infostream << "Connect aborted [Escape]" << std::endl;
				break;
			}

			wait_time += dtime;
			if (local_server_mode) {
				// never time out
			} else if (wait_time > GAME_FALLBACK_TIMEOUT && !did_fallback) {
                if (!pkt_handler->hasServerReplied() && fallback_address.isValid()) {
                    pkt_handler->connect(fallback_address, address_name,
						simple_singleplayer_mode || local_server_mode);
				}
				did_fallback = true;
			} else if (wait_time > GAME_CONNECTION_TIMEOUT) {
				*error_message = gettext("Connection timed out.");
				errorstream << *error_message << std::endl;
				break;
			}

			// Update status
			showOverlayMessage(N_("Connecting to server..."), dtime, 20);
		}
		framemarker.end();
	} catch (con::PeerNotFoundException &e) {
		warningstream << "This should not happen. Please report a bug." << std::endl;
		return false;
	}

	return true;
}

bool Game::getServerContent(bool *aborted)
{
	input->clear();

	f32 dtime; // in seconds

    auto drawstats = rndsys->getRenderer()->getDrawStats();
    drawstats.fps.reset();

	auto framemarker = FrameMarker("Game::getServerContent()-frame").started();

    auto pkt_handler = client->getPacketHandler();

    while (rndsys->run()) {
		framemarker.end();
        drawstats.fps.limit(rndsys->getWindow(), &dtime);
		framemarker.start();

		// Update client and server
		step(dtime);

		// End condition
        if (client->mediaReceived() && pkt_handler->itemdefReceived() &&
                pkt_handler->nodedefReceived()) {
			return true;
		}

		// Error conditions
        if (!pkt_handler->checkConnection(error_message, reconnect_requested))
			return false;

		if (client->getState() < LC_Init) {
			*error_message = gettext("Client disconnected");
			errorstream << *error_message << std::endl;
			return false;
		}

		if (input->cancelPressed()) {
			*aborted = true;
			infostream << "Connect aborted [Escape]" << std::endl;
			return false;
		}

		// Display status
		int progress = 25;

        if (!pkt_handler->itemdefReceived()) {
			progress = 25;
            showOverlayMessage(N_("Item definitions..."), dtime, progress);
        } else if (!pkt_handler->nodedefReceived()) {
			progress = 30;
            showOverlayMessage(N_("Node definitions..."), dtime, progress);
		} else {
			std::ostringstream message;
			std::fixed(message);
			message.precision(0);
			float receive = client->mediaReceiveProgress() * 100;
			message << gettext("Media...");
			if (receive > 0)
				message << " " << receive << "%";
			message.precision(2);

			if ((USE_CURL == 0) ||
					(!g_settings->getBool("enable_remote_media_server"))) {
                float cur = pkt_handler->getCurRate();
				std::string cur_unit = gettext("KiB/s");

				if (cur > 900) {
					cur /= 1024.0;
					cur_unit = gettext("MiB/s");
				}

				message << " (" << cur << ' ' << cur_unit << ")";
			}

			progress = 30 + client->mediaReceiveProgress() * 35 + 0.5;
            showOverlayMessage(N_(message.str().c_str()), dtime, progress);
		}
	}
	framemarker.end();

	*aborted = true;
	infostream << "Connect aborted [device]" << std::endl;
	return false;
}


/****************************************************************************/
/****************************************************************************
 Run
 ****************************************************************************/
/****************************************************************************/

void Game::updatePauseState()
{
	bool was_paused = m_is_paused;
	m_is_paused = simple_singleplayer_mode && g_menumgr->pausesGame();

	if (!was_paused && m_is_paused) {
		client->getSoundManager()->pauseAll();
	} else if (was_paused && !m_is_paused) {
		client->getSoundManager()->resumeAll();
	}
}

void Game::step(f32 dtime)
{
	ZoneScoped;

	if (server) {
        float fps_max = (!rndsys->getWindow()->isFocused() || g_menumgr->pausesGame()) ?
				g_settings->getFloat("fps_max_unfocused") :
				g_settings->getFloat("fps_max");
		fps_max = std::max(fps_max, 1.0f);
		/*
		 * Unless you have a barebones game, running the server at more than 60Hz
		 * is hardly realistic and you're at the point of diminishing returns.
		 * fps_max is also not necessarily anywhere near the FPS actually achieved
		 * (also due to vsync).
		 */
		fps_max = std::min(fps_max, 60.0f);

		server->setStepSettings(Server::StepSettings{
				1.0f / fps_max,
				m_is_paused
			});

		server->step();
	}

	if (!m_is_paused)
        client->step(dtime);
}

/****************************************************************************
 * Shadows
 *****************************************************************************/
/*void Game::updateShadows()
{
	ShadowRenderer *shadow = RenderingEngine::get_shadow_renderer();
	if (!shadow)
		return;

	float in_timeofday = std::fmod(runData.time_of_day_smooth, 1.0f);

	float timeoftheday = getWickedTimeOfDay(in_timeofday);
	bool is_day = timeoftheday > 0.25 && timeoftheday < 0.75;
	bool is_shadow_visible = is_day ? sky->getSunVisible() : sky->getMoonVisible();
	const auto &lighting = client->getEnv().getLocalPlayer()->getLighting();
	shadow->setShadowIntensity(is_shadow_visible ? lighting.shadow_intensity : 0.0f);
	shadow->setShadowTint(lighting.shadow_tint);

	timeoftheday = std::fmod(timeoftheday + 0.75f, 0.5f) + 0.25f;
	const float offset_constant = 10000.0f;

	v3f light = is_day ? sky->getSunDirection() : sky->getMoonDirection();

	v3f sun_pos = light * offset_constant;

	shadow->getDirectionalLight().setDirection(sun_pos);
	shadow->setTimeOfDay(in_timeofday);

	shadow->getDirectionalLight().update_frustum(camera, client, m_camera_offset_changed);
}*/

/****************************************************************************
 Misc
 ****************************************************************************/

void Game::showOverlayMessage(const char *msg, float dtime, int percent, float *indef_pos)
{
    rndsys->getLoadScreen()->draw(rndsys->getWindowSize(), wstrgettext(msg),
        dtime, g_settings->getBool("menu_clouds"), percent, rndsys->getScaleFactor(), indef_pos);
}

void Game::settingChangedCallback(const std::string &setting_name, void *data)
{
	((Game *)data)->readSettings();
}

void Game::readSettings()
{
	m_does_lost_focus_pause_game = g_settings->getBool("pause_on_lost_focus");
}

/****************************************************************************/
/****************************************************************************
 extern function for launching the game
 ****************************************************************************/
/****************************************************************************/

void the_game(bool *kill,
        InputHandler *input,
        RenderSystem *rndsys,
        ResourceCache *rescache,
        const GameStartData &start_data,
        std::string &error_message,
        bool *reconnect_requested) // Used for local game
{
	Game game;

	/* Make a copy of the server address because if a local singleplayer server
	 * is created then this is updated and we don't want to change the value
	 * passed to us by the calling function
	 */

	try {

        if (game.startup(kill, input, rndsys, rescache, start_data,
                error_message, reconnect_requested)) {
			game.run();
		}

	} catch (SerializationError &e) {
		const std::string ver_err = fmtgettext("The server is probably running a different version of %s.", PROJECT_NAME_C);
		error_message = strgettext("A serialization error occurred:") +"\n"
				+ e.what() + "\n\n" + ver_err;
		errorstream << error_message << std::endl;
	} catch (ServerError &e) {
		error_message = e.what();
		errorstream << "ServerError: " << error_message << std::endl;
	} catch (ModError &e) {
		// DO NOT TRANSLATE the `ModError`, it's used by `ui.lua`
		error_message = std::string("ModError: ") + e.what() +
				strgettext("\nCheck debug.txt for details.");
		errorstream << error_message << std::endl;
	} catch (con::PeerNotFoundException &e) {
		error_message = gettext("Connection error (timed out?)");
		errorstream << error_message << std::endl;
	} catch (ShaderException &e) {
		error_message = e.what();
		errorstream << error_message << std::endl;
	}

	game.shutdown();
}
