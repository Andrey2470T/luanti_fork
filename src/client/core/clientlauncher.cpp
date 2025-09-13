// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#include "gui/mainmenumanager.h"
#include "client/render/clouds.h"
#include "gui/touchcontrols.h"
#include "filesys.h"
#include "gui/guiMainMenu.h"
#include "game.h"
#include "player.h"
#include "chat.h"
#include "gettext.h"
#include "client/event/inputhandler.h"
#include "profiler.h"
#include "gui/guiEngine.h"
#include "client/ui/glyph_atlas.h"
#include "clientlauncher.h"
#include "version.h"
#include "client/render/rendersystem.h"
#include "settings.h"
#include "util/tracy_wrapper.h"
#include <unordered_map>
#include "client/media/resource.h"

#if USE_SOUND
    #include "client/sound/soundopenal.h"
#endif

/* mainmenumanager.h
 */
std::unique_ptr<MainMenuManager> g_menumgr;

// Passed to menus to allow disconnecting and exiting
MainGameCallback *g_gamecallback = nullptr;

#if 0
// This can be helpful for the next code cleanup
static void dump_start_data(const GameStartData &data)
{
    std::cout <<
        "\ndedicated   " << (int)data.is_dedicated_server <<
        "\nport        " << data.socket_port <<
        "\nworld_path  " << data.world_spec.path <<
        "\nworld game  " << data.world_spec.gameid <<
        "\ngame path   " << data.game_spec.path <<
        "\nplayer name " << data.name <<
        "\naddress     " << data.address << std::endl;
}
#endif

ClientLauncher::~ClientLauncher()
{
    g_settings->deregisterAllChangedCallbacks(this);

    delete g_gamecallback;
    g_gamecallback = nullptr;

    if (g_menumgr)
        assert(g_menumgr->menuCount() == 0);

    g_menumgr.reset();

#if USE_SOUND
    g_sound_manager_singleton.reset();
#endif
}


bool ClientLauncher::run(GameStartData &start_data, const Settings &cmd_args)
{
    /* This function is called when a client must be started.
     * Covered cases:
     *   - Singleplayer (address but map provided)
     *   - Join server (no map but address provided)
     *   - Local server (for main menu only)
    */

    init_args(start_data, cmd_args);

#if USE_SOUND
    g_sound_manager_singleton = createSoundManagerSingleton();
#endif

    if (!init_engine())
        return false;

    render_system->setWindowIcon();

    // Create game callback for menus
    g_gamecallback = new MainGameCallback();

    render_system->setResizable(true);

    render_system->buildGUIAtlas();

    init_input();

    config_guienv();

    g_settings->registerChangedCallback("dpi_change_notifier", setting_changed_callback, this);
    g_settings->registerChangedCallback("display_density_factor", setting_changed_callback, this);
    g_settings->registerChangedCallback("gui_scaling", setting_changed_callback, this);

    g_menumgr = std::make_unique<MainMenuManager>(render_system.get(), resource_cache.get());

    // If an error occurs, this is set to something by menu().
    // It is then displayed before the menu shows on the next call to menu()
    std::string error_message;
    bool reconnect_requested = false;

    bool first_loop = true;

    /*
        Menu-game loop
    */
    bool retval = true;
    bool *kill = porting::signal_handler_killstatus();

    while (render_system->run() && !*kill &&
        !g_gamecallback->shutdown_requested) {
        // Set the window caption
        auto gl_version = render_system->getWindow()->getGLVersionString();
        std::string caption = std::string(PROJECT_NAME_C) +
            " " + g_version_hash +
            " [" + gettext("Main Menu") + "]" +
            " [" + gl_version + "]";

        render_system->getWindow()->setCaption(utf8_to_wide(caption));

#ifdef NDEBUG
        try {
#endif
            render_system->getGUIEnvironment()->clear();

            /*
                We need some kind of a root node to be able to add
                custom gui elements directly on the screen.
                Otherwise they won't be automatically drawn.
            */
            render_system->getGUIEnvironment()->addStaticText(L"",
               recti(0, 0, 10000, 10000));

            bool should_run_game = launch_game(error_message, reconnect_requested,
                start_data, cmd_args);

            // Reset the reconnect_requested flag
            reconnect_requested = false;

            // If skip_main_menu, we only want to startup once
            if (skip_main_menu && !first_loop)
                break;
            first_loop = false;

            if (!should_run_game) {
                if (skip_main_menu)
                    break;
                continue;
            }

            // Break out of menu-game loop to shut down cleanly
            if (!render_system->run() || *kill)
                break;

            the_game(
                kill,
                input.get(),
                render_system.get(),
                resource_cache.get(),
                start_data,
                error_message,
                &reconnect_requested
            );
#ifdef NDEBUG
        } catch (std::exception &e) {
            error_message = "Some exception: ";
            error_message.append(debug_describe_exc(e));
            errorstream << error_message << std::endl;
        }
#endif

        //render_system->get_scene_manager()->clear();

        if (g_touchcontrols) {
            delete g_touchcontrols;
            g_touchcontrols = NULL;
        }

        /* Save the settings when leaving the game.
         * This makes sure that setting changes made in-game are persisted even
         * in case of a later unclean exit from the mainmenu.
         * This is especially useful on Android because closing the app from the
         * "Recents screen" results in an unclean exit.
         * Caveat: This means that the settings are saved twice when exiting Minetest.
         */
        if (!g_settings_path.empty())
            g_settings->updateConfigFile(g_settings_path.c_str());

        // If no main menu, show error and exit
        if (skip_main_menu) {
            if (!error_message.empty())
                retval = false;
            break;
        }
    } // Menu-game loop

    // If profiler was enabled print it one last time
    if (g_settings->getFloat("profiler_print_interval") > 0) {
        infostream << "Profiler:" << std::endl;
        g_profiler->print(infostream);
        g_profiler->clear();
    }

    return retval;
}

void ClientLauncher::init_args(GameStartData &start_data, const Settings &cmd_args)
{
    skip_main_menu = cmd_args.getFlag("go");

    start_data.address = g_settings->get("address");
    if (cmd_args.exists("address")) {
        // Join a remote server
        start_data.address = cmd_args.get("address");
        start_data.world_path.clear();
        start_data.name = g_settings->get("name");
    }
    if (!start_data.world_path.empty()) {
        // Start a singleplayer instance
        start_data.address = "";
    }

    if (cmd_args.exists("name"))
        start_data.name = cmd_args.get("name");

    random_input = g_settings->getBool("random_input")
            || cmd_args.getFlag("random-input");
}

bool ClientLauncher::init_engine()
{
    resource_cache = std::make_unique<ResourceCache>();
    try {
        render_system = std::make_unique<RenderSystem>(resource_cache.get());
        receiver = std::make_unique<MyEventReceiver>(render_system->getWindow());
    } catch (std::exception &e) {
        errorstream << e.what() << std::endl;
    }

    return !!render_system;
}

void ClientLauncher::init_input()
{
    if (random_input)
        input = std::make_unique<RandomInputHandler>(receiver.get());
    else
        input = std::make_unique<RealInputHandler>(receiver.get());

    if (g_settings->getBool("enable_joysticks"))
        init_joysticks();
}

void ClientLauncher::init_joysticks()
{
    std::vector<core::JoystickInfo> infos;
    std::vector<core::JoystickInfo> joystick_infos;

    // Make sure this is called maximum once per
    // irrlicht device, otherwise it will give you
    // multiple events for the same joystick.
    if (!render_system->getWindow()->activateJoysticks(infos)) {
        errorstream << "Could not activate joystick support." << std::endl;
        return;
    }

    infostream << "Joystick support enabled" << std::endl;
    joystick_infos.reserve(infos.size());
    for (u32 i = 0; i < infos.size(); i++) {
        joystick_infos.push_back(infos[i]);
    }
    input->joystick.onJoystickConnect(joystick_infos);
}

void ClientLauncher::setting_changed_callback(const std::string &name, void *data)
{
    static_cast<ClientLauncher*>(data)->config_guienv();
}

void ClientLauncher::config_guienv()
{
    GUISkin *skin = render_system->getGUIEnvironment()->getSkin();

    skin->setColor(GUIDefaultColor::WindowSymbol, img::white);
    skin->setColor(GUIDefaultColor::ButtonText, img::white);
    skin->setColor(GUIDefaultColor::Light3D, img::black);
    skin->setColor(GUIDefaultColor::HighLight3D, img::color8(img::PF_RGBA8, 30, 30, 30, 255));
    skin->setColor(GUIDefaultColor::Shadow3D, img::black);
    skin->setColor(GUIDefaultColor::HighLight, img::color8(img::PF_RGBA8, 70, 120, 50, 255));
    skin->setColor(GUIDefaultColor::HighLightText, img::white);
    skin->setColor(GUIDefaultColor::Editable, img::color8(img::PF_RGBA8, 128, 128, 128, 255));
    skin->setColor(GUIDefaultColor::FocusedEditable, img::color8(img::PF_RGBA8, 96, 134, 49, 255));

    float density = render_system->getDisplayDensity();
    skin->setScale(density);
    skin->setSize(GUIDefaultSize::CheckBoxWidth, (s32)(17.0f * density));
    skin->setSize(GUIDefaultSize::ScrollbarSize, (s32)(21.0f * density));
    skin->setSize(GUIDefaultSize::WindowButtonWidth, (s32)(15.0f * density));

    static u32 orig_sprite_id = skin->getIcon(GUIDefaultIcon::CheckBoxChecked);
    static std::unordered_map<std::string, u32> sprite_ids;

    if (density > 1.5f) {
        // Texture dimensions should be a power of 2
        std::string path = porting::path_share + "/textures/base/pack/";
        if (density > 3.5f)
            path.append("checkbox_64.png");
        else if (density > 2.0f)
            path.append("checkbox_32.png");
        else
            path.append("checkbox_16.png");

        auto cached_id = sprite_ids.find(path);
        if (cached_id != sprite_ids.end()) {
            skin->setIcon(GUIDefaultIcon::CheckBoxChecked, cached_id->second);
        } else {
            gui::IGUISpriteBank *sprites = skin->getSpriteBank();
            img::Image *texture = resource_cache->getOrLoad<img::Image>(ResourceType::IMAGE, path);
            s32 id = sprites->addTextureAsSprite(texture);
            if (id != -1) {
                skin->setIcon(GUIDefaultIcon::CheckBoxChecked, id);
                sprite_ids.emplace(path, id);
            }
        }
    } else {
        skin->setIcon(GUIDefaultIcon::CheckBoxChecked, orig_sprite_id);
    }
}

bool ClientLauncher::launch_game(std::string &error_message,
        bool reconnect_requested, GameStartData &start_data,
        const Settings &cmd_args)
{
    // Prepare and check the start data to launch a game
    std::string error_message_lua = error_message;
    error_message.clear();

    if (cmd_args.exists("password"))
        start_data.password = cmd_args.get("password");

    if (cmd_args.exists("password-file")) {
        std::ifstream passfile(cmd_args.get("password-file"));
        if (passfile.good()) {
            std::getline(passfile, start_data.password);
        } else {
            error_message = gettext("Provided password file "
                    "failed to open: ")
                    + cmd_args.get("password-file");
            errorstream << error_message << std::endl;
            return false;
        }
    }

    // If a world was commanded, append and select it
    // This is provieded by "get_world_from_cmdline()", main.cpp
    if (!start_data.world_path.empty()) {
        auto &spec = start_data.world_spec;

        spec.path = start_data.world_path;
        spec.gameid = getWorldGameId(spec.path, true);
        spec.name = _("[--world parameter]");
    }

    /* Show the GUI menu
     */
    std::string server_name, server_description;
    if (!skip_main_menu) {
        // Initialize menu data
        // TODO: Re-use existing structs (GameStartData)
        MainMenuData menudata;
        menudata.address                         = start_data.address;
        menudata.name                            = start_data.name;
        menudata.password                        = start_data.password;
        menudata.port                            = itos(start_data.socket_port);
        menudata.script_data.errormessage        = std::move(error_message_lua);
        menudata.script_data.reconnect_requested = reconnect_requested;

        main_menu(&menudata);

        // Skip further loading if there was an exit signal.
        if (*porting::signal_handler_killstatus())
            return false;

        if (!menudata.script_data.errormessage.empty()) {
            /* The calling function will pass this back into this function upon the
             * next iteration (if any) causing it to be displayed by the GUI
             */
            error_message = menudata.script_data.errormessage;
            return false;
        }

        int newport = stoi(menudata.port);
        if (newport != 0)
            start_data.socket_port = newport;

        // Update world information using main menu data
        std::vector<WorldSpec> worldspecs = getAvailableWorlds();

        int world_index = menudata.selected_world;
        if (world_index >= 0 && world_index < (int)worldspecs.size()) {
            start_data.world_spec = worldspecs[world_index];
        }

        start_data.name = menudata.name;
        start_data.password = menudata.password;
        start_data.address = std::move(menudata.address);
        start_data.allow_login_or_register = menudata.allow_login_or_register;
        server_name = menudata.servername;
        server_description = menudata.serverdescription;

        start_data.local_server = !menudata.simple_singleplayer_mode &&
            start_data.address.empty();
    } else {
        start_data.local_server = !start_data.world_path.empty() &&
            start_data.address.empty() && !start_data.name.empty();
    }

    if (!render_system->run())
        return false;

    if (!start_data.isSinglePlayer() && start_data.name.empty()) {
        error_message = gettext("Please choose a name!");
        errorstream << error_message << std::endl;
        return false;
    }

    // If using simple singleplayer mode, override
    if (start_data.isSinglePlayer()) {
        start_data.name = "singleplayer";
        start_data.password = "";
        start_data.socket_port = myrand_range(49152, 65535);
    } else {
        g_settings->set("name", start_data.name);
    }

    if (start_data.name.length() > PLAYERNAME_SIZE - 1) {
        error_message = gettext("Player name too long.");
        start_data.name.resize(PLAYERNAME_SIZE);
        g_settings->set("name", start_data.name);
        return false;
    }

    auto &worldspec = start_data.world_spec;
    infostream << "Selected world: " << worldspec.name
               << " [" << worldspec.path << "]" << std::endl;

    if (start_data.address.empty()) {
        // For singleplayer and local server
        if (worldspec.path.empty()) {
            error_message = gettext("No world selected and no address "
                    "provided. Nothing to do.");
            errorstream << error_message << std::endl;
            return false;
        }

        if (!mt_fs::PathExists(worldspec.path)) {
            error_message = gettext("Provided world path doesn't exist: ")
                    + worldspec.path;
            errorstream << error_message << std::endl;
            return false;
        }

        // Load gamespec for required game
        start_data.game_spec = findWorldSubgame(worldspec.path);
        if (!start_data.game_spec.isValid()) {
            error_message = gettext("Could not find or load game: ")
                    + worldspec.gameid;
            errorstream << error_message << std::endl;
            return false;
        }

        return true;
    }

    start_data.world_path = start_data.world_spec.path;
    return true;
}

void ClientLauncher::main_menu(MainMenuData *menudata)
{
    bool *kill = porting::signal_handler_killstatus();

    infostream << "Waiting for other menus" << std::endl;
    auto framemarker = FrameMarker("ClientLauncher::main_menu()-wait-frame").started();

    auto ctxt = render_system->getRenderer()->getContext();
    while (render_system->run() && !*kill) {
        if (!isMenuActive())
            break;
        ctxt->clearBuffers(render::CBF_COLOR | render::CBF_DEPTH, img::gray);
        render_system->getGUIEnvironment()->drawAll();
        framemarker.end();
        // On some computers framerate doesn't seem to be automatically limited
        sleep_ms(25);
        framemarker.start();
    }
    framemarker.end();
    infostream << "Waited for other menus" << std::endl;

    auto *cur_control = render_system->getWindow()->getCursorControl();
    if (cur_control) {
        // Cursor can be non-visible when coming from the game
        cur_control->setVisible(true);
        // Set absolute mouse mode
        cur_control->setRelativeMode(false);
    }

    /* show main menu */
    GUIEngine mymenu(&input->joystick, guiroot, render_system, &g_menumgr, menudata, *kill);

    /* leave scene manager in a clean state */
    render_system->get_scene_manager()->clear();

    /* Save the settings when leaving the mainmenu.
     * This makes sure that setting changes made in the mainmenu are persisted
     * even in case of a later unclean exit from the game.
     * This is especially useful on Android because closing the app from the
     * "Recents screen" results in an unclean exit.
     * Caveat: This means that the settings are saved twice when exiting Minetest.
     */
    if (!g_settings_path.empty())
        g_settings->updateConfigFile(g_settings_path.c_str());
}
