// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#pragma once

#include <string>
#include <memory>

class Settings;
struct GameStartData;
struct MainMenuData;
class RenderSystem;
class IEventReceiver;
class InputHandler;
class ResourceCache;

class ClientLauncher
{
public:
    ClientLauncher() = default;

    ~ClientLauncher();

    bool run(GameStartData &start_data, const Settings &cmd_args);

private:
    void init_args(GameStartData &start_data, const Settings &cmd_args);
    bool init_engine();
    void init_input();
    void init_joysticks();

    static void setting_changed_callback(const std::string &name, void *data);
    void config_guienv();

    bool launch_game(std::string &error_message, bool reconnect_requested,
        GameStartData &start_data, const Settings &cmd_args);

    void main_menu(MainMenuData *menudata);

    bool skip_main_menu = false;
    bool random_input = false;

    std::unique_ptr<ResourceCache> resource_cache;
    std::unique_ptr<RenderSystem> render_system;
    std::unique_ptr<IEventReceiver> receiver;
    std::unique_ptr<InputHandler> input;
};
