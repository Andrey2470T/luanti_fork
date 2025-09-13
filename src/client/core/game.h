// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#pragma once

#include <BasicIncludes.h>
#include "clientdynamicinfo.h"
#include "config.h"
#include <string>
#include <memory>

#if !IS_CLIENT_BUILD
#error Do not include in server builds
#endif

struct SubgameSpec;
struct GameStartData;
class Client;
class Server;
class InputHandler;
class ResourceCache;
class RenderSystem;

#define GAME_FALLBACK_TIMEOUT 1.8f
#define GAME_CONNECTION_TIMEOUT 10.0f


/****************************************************************************
 THE GAME
 ****************************************************************************/

//using PausedNodesList = std::vector<std::pair<irr_ptr<scene::IAnimatedMeshSceneNode>, float>>;

/* This is not intended to be a public class. If a public class becomes
 * desirable then it may be better to create another 'wrapper' class that
 * hides most of the stuff in this class (nothing in this class is required
 * by any other file) but exposes the public methods/data only.
 */
class Game {
public:
    Game();
    ~Game();

    bool startup(bool *kill,
            InputHandler *input,
            RenderSystem *rndsys,
            ResourceCache *rescache,
            const GameStartData &game_params,
            std::string &error_message,
            bool *reconnect);

    void run();
    void shutdown();

    bool createSingleplayerServer(const std::string &map_dir,
            const SubgameSpec &gamespec, u16 port);
    void copyServerClientCache();

    // Client creation
    bool createClient(const GameStartData &start_data);

    // Client connection
    bool connectToServer(const GameStartData &start_data,
            bool *connect_ok, bool *aborted);
    bool getServerContent(bool *aborted);

    // Main loop

    void updatePauseState();

    void step(f32 dtime);

    // Misc
    void showOverlayMessage(const char *msg, f32 dtime, int percent,
            f32 *indef_pos = nullptr);

    static void settingChangedCallback(const std::string &setting_name, void *data);
    void readSettings();

private:
    std::unique_ptr<Client> client;
    std::unique_ptr<Server> server;

    ClientDynamicInfo client_display_info{};
    f32 dynamic_info_send_timer = 0;

    RenderSystem *rndsys;
    ResourceCache *rescache;
    InputHandler *input;

    std::string *error_message = nullptr;
    bool *reconnect_requested = nullptr;

    /* 'cache'
       This class does take ownership/responsibily for cleaning up etc of any of
       these items (e.g. device)
    */
    bool *kill;
    bool simple_singleplayer_mode;
    /* End 'cache' */

    bool m_does_lost_focus_pause_game = false;

    // if true, (almost) the whole game is paused
    // this happens in pause menu in singleplayer
    bool m_is_paused = false;

    f32 m_shutdown_progress = 0.0f;
};

void the_game(bool *kill,
        InputHandler *input,
        RenderSystem *rndsys,
        ResourceCache *rescache,
		const GameStartData &start_data,
		std::string &error_message,
		bool *reconnect_requested);
