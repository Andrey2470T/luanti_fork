#include "gameinputhandler.h"
#include "client/event/inputhandler.h"
#include "client/event/eventreceiver.h"
#include "client/render/rendersystem.h"
#include <Core/MainWindow.h>
#include "settings.h"

GameInputSystem::GameInputSystem(Client *_client, bool random_input)
    : client(_client), receiver(std::make_unique<MtEventReceiver>(client->getRenderSystem()->getWindow()))
{
    if (random_input)
        input = std::make_unique<RandomInputHandler>();
    else
        input = std::make_unique<RealInputHandler>(receiver.get());

    if (g_settings->getBool("enable_joysticks")) {
        std::vector<core::JoystickInfo> infos;
        std::vector<core::JoystickInfo> joystick_infos;

        // Make sure this is called maximum once per
        // irrlicht device, otherwise it will give you
        // multiple events for the same joystick.
        if (!client->getRenderSystem()->getWindow()->activateJoysticks(infos)) {
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


}

void GameInputSystem::processUserInput(f32 dtime)
{
    bool desired = shouldShowTouchControls();
    if (desired && !g_touchcontrols) {
        g_touchcontrols = new TouchControls(device, texture_src);

    } else if (!desired && g_touchcontrols) {
        delete g_touchcontrols;
        g_touchcontrols = nullptr;
    }

    // Reset input if window not active or some menu is active
    if (!device->isWindowActive() || isMenuActive() || guienv->hasFocus(gui_chat_console.get())) {
        if (m_GameInputSystem_focused) {
            m_GameInputSystem_focused = false;
            infostream << "GameInputSystem lost focus" << std::endl;
            input->releaseAllKeys();
        } else {
            input->clear();
        }

        if (g_touchcontrols)
            g_touchcontrols->hide();

    } else {
        if (g_touchcontrols) {
            /* on touchcontrols step may generate own input events which ain't
             * what we want in case we just did clear them */
            g_touchcontrols->show();
            g_touchcontrols->step(dtime);
        }

        m_GameInputSystem_focused = true;
    }

    if (!guienv->hasFocus(gui_chat_console.get()) && gui_chat_console->isOpen()
        && !gui_chat_console->isMyChild(guienv->getFocus()))
    {
        gui_chat_console->closeConsoleAtOnce();
    }

    // Input handler step() (used by the random input generator)
    input->step(dtime);

#ifdef __ANDROID__
    if (!m_GameInputSystem_formspec.handleAndroidUIInput())
        handleAndroidChatInput();
#endif

    // Increase timer for double tap of "keymap_jump"
    if (m_cache_doubletap_jump && runData.jump_timer_up <= 0.2f)
        runData.jump_timer_up += dtime;
    if (m_cache_doubletap_jump && runData.jump_timer_down <= 0.4f)
        runData.jump_timer_down += dtime;

    processKeyInput();
    processItemSelection(&runData.new_playeritem);
}


void GameInputSystem::processKeyInput()
{
    if (wasKeyDown(KeyType::DROP)) {
        dropSelectedItem(isKeyDown(KeyType::SNEAK));
    } else if (wasKeyDown(KeyType::AUTOFORWARD)) {
        toggleAutoforward();
    } else if (wasKeyDown(KeyType::BACKWARD)) {
        if (g_settings->getBool("continuous_forward"))
            toggleAutoforward();
    } else if (wasKeyDown(KeyType::INVENTORY)) {
        m_GameInputSystem_formspec.showPlayerInventory();
    } else if (input->cancelPressed()) {
#ifdef __ANDROID__
        m_android_chat_open = false;
#endif
        if (!gui_chat_console->isOpenInhibited()) {
            m_GameInputSystem_formspec.showPauseMenu();
        }
    } else if (wasKeyDown(KeyType::CHAT)) {
        openConsole(0.2, L"");
    } else if (wasKeyDown(KeyType::CMD)) {
        openConsole(0.2, L"/");
    } else if (wasKeyDown(KeyType::CMD_LOCAL)) {
        if (client->modsLoaded())
            openConsole(0.2, L".");
        else
            m_GameInputSystem_ui->showTranslatedStatusText("Client side scripting is disabled");
    } else if (wasKeyDown(KeyType::CONSOLE)) {
        openConsole(core::clamp(g_settings->getFloat("console_height"), 0.1f, 1.0f));
    } else if (wasKeyDown(KeyType::FREEMOVE)) {
        toggleFreeMove();
    } else if (wasKeyDown(KeyType::JUMP)) {
        toggleFreeMoveAlt();
    } else if (wasKeyDown(KeyType::PITCHMOVE)) {
        togglePitchMove();
    } else if (wasKeyDown(KeyType::FASTMOVE)) {
        toggleFast();
    } else if (wasKeyDown(KeyType::NOCLIP)) {
        toggleNoClip();
#if USE_SOUND
    } else if (wasKeyDown(KeyType::MUTE)) {
        bool new_mute_sound = !g_settings->getBool("mute_sound");
        g_settings->setBool("mute_sound", new_mute_sound);
        if (new_mute_sound)
            m_GameInputSystem_ui->showTranslatedStatusText("Sound muted");
        else
            m_GameInputSystem_ui->showTranslatedStatusText("Sound unmuted");
    } else if (wasKeyDown(KeyType::INC_VOLUME)) {
        float new_volume = g_settings->getFloat("sound_volume", 0.0f, 0.9f) + 0.1f;
        g_settings->setFloat("sound_volume", new_volume);
        std::wstring msg = fwgettext("Volume changed to %d%%", myround(new_volume * 100));
        m_GameInputSystem_ui->showStatusText(msg);
    } else if (wasKeyDown(KeyType::DEC_VOLUME)) {
        float new_volume = g_settings->getFloat("sound_volume", 0.1f, 1.0f) - 0.1f;
        g_settings->setFloat("sound_volume", new_volume);
        std::wstring msg = fwgettext("Volume changed to %d%%", myround(new_volume * 100));
        m_GameInputSystem_ui->showStatusText(msg);
#else
    } else if (wasKeyDown(KeyType::MUTE) || wasKeyDown(KeyType::INC_VOLUME)
            || wasKeyDown(KeyType::DEC_VOLUME)) {
        m_GameInputSystem_ui->showTranslatedStatusText("Sound system is not supported on this build");
#endif
    } else if (wasKeyDown(KeyType::CINEMATIC)) {
        toggleCinematic();
    } else if (wasKeyPressed(KeyType::SCREENSHOT)) {
        client->makeScreenshot();
    } else if (wasKeyPressed(KeyType::TOGGLE_BLOCK_BOUNDS)) {
        toggleBlockBounds();
    } else if (wasKeyPressed(KeyType::TOGGLE_HUD)) {
        m_GameInputSystem_ui->toggleHud();
    } else if (wasKeyPressed(KeyType::MINIMAP)) {
        toggleMinimap(isKeyDown(KeyType::SNEAK));
    } else if (wasKeyPressed(KeyType::TOGGLE_CHAT)) {
        m_GameInputSystem_ui->toggleChat(client);
    } else if (wasKeyPressed(KeyType::TOGGLE_FOG)) {
        toggleFog();
    } else if (wasKeyDown(KeyType::TOGGLE_UPDATE_CAMERA)) {
        toggleUpdateCamera();
    } else if (wasKeyPressed(KeyType::TOGGLE_DEBUG)) {
        toggleDebug();
    } else if (wasKeyPressed(KeyType::TOGGLE_PROFILER)) {
        m_GameInputSystem_ui->toggleProfiler();
    } else if (wasKeyDown(KeyType::INCREASE_VIEWING_RANGE)) {
        increaseViewRange();
    } else if (wasKeyDown(KeyType::DECREASE_VIEWING_RANGE)) {
        decreaseViewRange();
    } else if (wasKeyPressed(KeyType::RANGESELECT)) {
        toggleFullViewRange();
    } else if (wasKeyDown(KeyType::ZOOM)) {
        checkZoomEnabled();
    } else if (wasKeyDown(KeyType::QUICKTUNE_NEXT)) {
        quicktune->next();
    } else if (wasKeyDown(KeyType::QUICKTUNE_PREV)) {
        quicktune->prev();
    } else if (wasKeyDown(KeyType::QUICKTUNE_INC)) {
        quicktune->inc();
    } else if (wasKeyDown(KeyType::QUICKTUNE_DEC)) {
        quicktune->dec();
    }

    if (!isKeyDown(KeyType::JUMP) && runData.reset_jump_timer) {
        runData.reset_jump_timer = false;
        runData.jump_timer_up = 0.0f;
    }

    if (quicktune->hasMessage()) {
        m_GameInputSystem_ui->showStatusText(utf8_to_wide(quicktune->getMessage()));
    }
}

void GameInputSystem::processItemSelection(u16 *new_playeritem)
{
    LocalPlayer *player = client->getEnv().getLocalPlayer();

    *new_playeritem = player->getWieldIndex();
    u16 max_item = player->getMaxHotbarItemcount();
    if (max_item == 0)
        return;
    max_item -= 1;

    /* Item selection using mouse wheel
     */
    s32 wheel = input->getMouseWheel();
    if (!m_enable_hotbar_mouse_wheel)
        wheel = 0;
    if (m_invert_hotbar_mouse_wheel)
        wheel *= -1;

    s32 dir = wheel;

    if (wasKeyDown(KeyType::HOTBAR_NEXT))
        dir = -1;

    if (wasKeyDown(KeyType::HOTBAR_PREV))
        dir = 1;

    if (dir < 0)
        *new_playeritem = *new_playeritem < max_item ? *new_playeritem + 1 : 0;
    else if (dir > 0)
        *new_playeritem = *new_playeritem > 0 ? *new_playeritem - 1 : max_item;
    // else dir == 0

    /* Item selection using hotbar slot keys
     */
    for (u16 i = 0; i <= max_item; i++) {
        if (wasKeyDown((GameInputSystemKeyType) (KeyType::SLOT_1 + i))) {
            *new_playeritem = i;
            break;
        }
    }

    if (g_touchcontrols) {
        std::optional<u16> selection = g_touchcontrols->getHotbarSelection();
        if (selection)
            *new_playeritem = *selection;
    }

    // Clamp selection again in case it wasn't changed but max_item was
    *new_playeritem = MYMIN(*new_playeritem, max_item);
}


void GameInputSystem::dropSelectedItem(bool single_item)
{
    IDropAction *a = new IDropAction();
    a->count = single_item ? 1 : 0;
    a->from_inv.setCurrentPlayer();
    a->from_list = "main";
    a->from_i = client->getEnv().getLocalPlayer()->getWieldIndex();
    client->inventoryAction(a);
}

void GameInputSystem::openConsole(float scale, const wchar_t *line)
{
    assert(scale > 0.0f && scale <= 1.0f);

#ifdef __ANDROID__
    if (!porting::hasPhysicalKeyboardAndroid()) {
        porting::showTextInputDialog("", "", 2);
        m_android_chat_open = true;
    } else {
#endif
    if (gui_chat_console->isOpenInhibited())
        return;
    gui_chat_console->openConsole(scale);
    if (line) {
        gui_chat_console->setCloseOnEnter(true);
        gui_chat_console->replaceAndAddToHistory(line);
    }
#ifdef __ANDROID__
    } // else
#endif
}

#ifdef __ANDROID__
void GameInputSystem::handleAndroidChatInput()
{
    // It has to be a text input
    if (m_android_chat_open && porting::getLastInputDialogType() == porting::TEXT_INPUT) {
        porting::AndroidDialogState dialogState = porting::getInputDialogState();
        if (dialogState == porting::DIALOG_INPUTTED) {
            std::string text = porting::getInputDialogMessage();
            client->typeChatMessage(utf8_to_wide(text));
        }
        if (dialogState != porting::DIALOG_SHOWN)
            m_android_chat_open = false;
    }
}
#endif

void GameInputSystem::toggleFreeMove()
{
    bool free_move = !g_settings->getBool("free_move");
    g_settings->set("free_move", bool_to_cstr(free_move));

    if (free_move) {
        if (client->checkPrivilege("fly")) {
            m_GameInputSystem_ui->showTranslatedStatusText("Fly mode enabled");
        } else {
            m_GameInputSystem_ui->showTranslatedStatusText("Fly mode enabled (note: no 'fly' privilege)");
        }
    } else {
        m_GameInputSystem_ui->showTranslatedStatusText("Fly mode disabled");
    }
}

void GameInputSystem::toggleFreeMoveAlt()
{
    if (!runData.reset_jump_timer) {
        runData.jump_timer_down_before = runData.jump_timer_down;
        runData.jump_timer_down = 0.0f;
    }

    // key down (0.2 s max.), then key up (0.2 s max.), then key down
    if (m_cache_doubletap_jump && runData.jump_timer_up < 0.2f &&
            runData.jump_timer_down_before < 0.4f) // 0.2 + 0.2
        toggleFreeMove();

    runData.reset_jump_timer = true;
}


void GameInputSystem::togglePitchMove()
{
    bool pitch_move = !g_settings->getBool("pitch_move");
    g_settings->set("pitch_move", bool_to_cstr(pitch_move));

    if (pitch_move) {
        m_GameInputSystem_ui->showTranslatedStatusText("Pitch move mode enabled");
    } else {
        m_GameInputSystem_ui->showTranslatedStatusText("Pitch move mode disabled");
    }
}


void GameInputSystem::toggleFast()
{
    bool fast_move = !g_settings->getBool("fast_move");
    bool has_fast_privs = client->checkPrivilege("fast");
    g_settings->set("fast_move", bool_to_cstr(fast_move));

    if (fast_move) {
        if (has_fast_privs) {
            m_GameInputSystem_ui->showTranslatedStatusText("Fast mode enabled");
        } else {
            m_GameInputSystem_ui->showTranslatedStatusText("Fast mode enabled (note: no 'fast' privilege)");
        }
    } else {
        m_GameInputSystem_ui->showTranslatedStatusText("Fast mode disabled");
    }

    m_touch_simulate_aux1 = fast_move && has_fast_privs;
}


void GameInputSystem::toggleNoClip()
{
    bool noclip = !g_settings->getBool("noclip");
    g_settings->set("noclip", bool_to_cstr(noclip));

    if (noclip) {
        if (client->checkPrivilege("noclip")) {
            m_GameInputSystem_ui->showTranslatedStatusText("Noclip mode enabled");
        } else {
            m_GameInputSystem_ui->showTranslatedStatusText("Noclip mode enabled (note: no 'noclip' privilege)");
        }
    } else {
        m_GameInputSystem_ui->showTranslatedStatusText("Noclip mode disabled");
    }
}

void GameInputSystem::toggleCinematic()
{
    bool cinematic = !g_settings->getBool("cinematic");
    g_settings->set("cinematic", bool_to_cstr(cinematic));

    if (cinematic)
        m_GameInputSystem_ui->showTranslatedStatusText("Cinematic mode enabled");
    else
        m_GameInputSystem_ui->showTranslatedStatusText("Cinematic mode disabled");
}

void GameInputSystem::toggleBlockBounds()
{
    LocalPlayer *player = client->getEnv().getLocalPlayer();
    if (!(client->checkPrivilege("debug") || (player->hud_flags & HUD_FLAG_BASIC_DEBUG))) {
        m_GameInputSystem_ui->showTranslatedStatusText("Can't show block bounds (disabled by GameInputSystem or mod)");
        return;
    }
    enum Hud::BlockBoundsMode newmode = hud->toggleBlockBounds();
    switch (newmode) {
        case Hud::BLOCK_BOUNDS_OFF:
            m_GameInputSystem_ui->showTranslatedStatusText("Block bounds hidden");
            break;
        case Hud::BLOCK_BOUNDS_CURRENT:
            m_GameInputSystem_ui->showTranslatedStatusText("Block bounds shown for current block");
            break;
        case Hud::BLOCK_BOUNDS_NEAR:
            m_GameInputSystem_ui->showTranslatedStatusText("Block bounds shown for nearby blocks");
            break;
        default:
            break;
    }
}

// Autoforward by toggling continuous forward.
void GameInputSystem::toggleAutoforward()
{
    bool autorun_enabled = !g_settings->getBool("continuous_forward");
    g_settings->set("continuous_forward", bool_to_cstr(autorun_enabled));

    if (autorun_enabled)
        m_GameInputSystem_ui->showTranslatedStatusText("Automatic forward enabled");
    else
        m_GameInputSystem_ui->showTranslatedStatusText("Automatic forward disabled");
}

void GameInputSystem::toggleMinimap(bool shift_pressed)
{
    if (!mapper || !m_GameInputSystem_ui->m_flags.show_hud || !g_settings->getBool("enable_minimap"))
        return;

    if (shift_pressed)
        mapper->toggleMinimapShape();
    else
        mapper->nextMode();

    // TODO: When legacy minimap is deprecated, keep only HUD minimap stuff here

    // Not so satisying code to keep compatibility with old fixed mode system
    // -->
    u32 hud_flags = client->getEnv().getLocalPlayer()->hud_flags;

    if (hud_flags & HUD_FLAG_MINIMAP_VISIBLE) {
    // If radar is disabled, try to find a non radar mode or fall back to 0
        if (!(hud_flags & HUD_FLAG_MINIMAP_RADAR_VISIBLE))
            while (mapper->getModeIndex() &&
                    mapper->getModeDef().type == MINIMAP_TYPE_RADAR)
                mapper->nextMode();
    }
    // <--
    // End of 'not so satifying code'
    if (hud && hud->hasElementOfType(HUD_ELEM_MINIMAP))
        m_GameInputSystem_ui->showStatusText(utf8_to_wide(mapper->getModeDef().label));
    else
        m_GameInputSystem_ui->showTranslatedStatusText("Minimap currently disabled by GameInputSystem or mod");
}

void GameInputSystem::toggleFog()
{
    bool flag = !g_settings->getBool("enable_fog");
    g_settings->setBool("enable_fog", flag);
    bool allowed = sky->getFogDistance() < 0 || client->checkPrivilege("debug");
    if (!allowed)
        m_GameInputSystem_ui->showTranslatedStatusText("Fog enabled by GameInputSystem or mod");
    else if (flag)
        m_GameInputSystem_ui->showTranslatedStatusText("Fog enabled");
    else
        m_GameInputSystem_ui->showTranslatedStatusText("Fog disabled");
}


void GameInputSystem::toggleDebug()
{
    LocalPlayer *player = client->getEnv().getLocalPlayer();
    bool has_debug = client->checkPrivilege("debug");
    bool has_basic_debug = has_debug || (player->hud_flags & HUD_FLAG_BASIC_DEBUG);

    // Initial: No debug info
    // 1x toggle: Debug text
    // 2x toggle: Debug text with profiler graph
    // 3x toggle: Debug text and wireframe (needs "debug" priv)
    // 4x toggle: Debug text and bbox (needs "debug" priv)
    //
    // The debug text can be in 2 modes: minimal and basic.
    // * Minimal: Only technical client info that not GameInputSystemplay-relevant
    // * Basic: Info that might give GameInputSystemplay advantage, e.g. pos, angle
    // Basic mode is used when player has the debug HUD flag set,
    // otherwise the Minimal mode is used.

    auto &state = m_flags.debug_state;
    state = (state + 1) % 5;
    if (state >= 3 && !has_debug)
        state = 0;

    m_GameInputSystem_ui->m_flags.show_minimal_debug = state > 0;
    m_GameInputSystem_ui->m_flags.show_basic_debug = state > 0 && has_basic_debug;
    m_GameInputSystem_ui->m_flags.show_profiler_graph = state == 2;
    draw_control->show_wireframe = state == 3;
    smgr->setGlobalDebugData(state == 4 ? bbox_debug_flag : 0,
            state == 4 ? 0 : bbox_debug_flag);

    if (state == 1) {
        m_GameInputSystem_ui->showTranslatedStatusText("Debug info shown");
    } else if (state == 2) {
        m_GameInputSystem_ui->showTranslatedStatusText("Profiler graph shown");
    } else if (state == 3) {
        if (driver->getDriverType() == video::EDT_OGLES2)
            m_GameInputSystem_ui->showTranslatedStatusText("Wireframe not supported by video driver");
        else
            m_GameInputSystem_ui->showTranslatedStatusText("Wireframe shown");
    } else if (state == 4) {
        m_GameInputSystem_ui->showTranslatedStatusText("Bounding boxes shown");
    } else {
        m_GameInputSystem_ui->showTranslatedStatusText("All debug info hidden");
    }
}


void GameInputSystem::toggleUpdateCamera()
{
    auto &flag = m_flags.disable_camera_update;
    flag = client->checkPrivilege("debug") ? !flag : false;
    if (flag)
        m_GameInputSystem_ui->showTranslatedStatusText("Camera update disabled");
    else
        m_GameInputSystem_ui->showTranslatedStatusText("Camera update enabled");
}


void GameInputSystem::increaseViewRange()
{
    s16 range = g_settings->getS16("viewing_range");
    s16 range_new = range + 10;
    s16 server_limit = sky->getFogDistance();

    if (range_new >= 4000) {
        range_new = 4000;
        std::wstring msg = server_limit >= 0 && range_new > server_limit ?
                fwgettext("Viewing range changed to %d (the maximum), but limited to %d by GameInputSystem or mod", range_new, server_limit) :
                fwgettext("Viewing range changed to %d (the maximum)", range_new);
        m_GameInputSystem_ui->showStatusText(msg);
    } else {
        std::wstring msg = server_limit >= 0 && range_new > server_limit ?
                fwgettext("Viewing range changed to %d, but limited to %d by GameInputSystem or mod", range_new, server_limit) :
                fwgettext("Viewing range changed to %d", range_new);
        m_GameInputSystem_ui->showStatusText(msg);
    }
    g_settings->set("viewing_range", itos(range_new));
}


void GameInputSystem::decreaseViewRange()
{
    s16 range = g_settings->getS16("viewing_range");
    s16 range_new = range - 10;
    s16 server_limit = sky->getFogDistance();

    if (range_new <= 20) {
        range_new = 20;
        std::wstring msg = server_limit >= 0 && range_new > server_limit ?
                fwgettext("Viewing changed to %d (the minimum), but limited to %d by GameInputSystem or mod", range_new, server_limit) :
                fwgettext("Viewing changed to %d (the minimum)", range_new);
        m_GameInputSystem_ui->showStatusText(msg);
    } else {
        std::wstring msg = server_limit >= 0 && range_new > server_limit ?
                fwgettext("Viewing range changed to %d, but limited to %d by GameInputSystem or mod", range_new, server_limit) :
                fwgettext("Viewing range changed to %d", range_new);
        m_GameInputSystem_ui->showStatusText(msg);
    }
    g_settings->set("viewing_range", itos(range_new));
}


void GameInputSystem::toggleFullViewRange()
{
    draw_control->range_all = !draw_control->range_all;
    if (draw_control->range_all) {
        if (sky->getFogDistance() >= 0) {
            m_GameInputSystem_ui->showTranslatedStatusText("Unlimited viewing range enabled, but forbidden by GameInputSystem or mod");
        } else {
            m_GameInputSystem_ui->showTranslatedStatusText("Unlimited viewing range enabled");
        }
    } else {
        m_GameInputSystem_ui->showTranslatedStatusText("Unlimited viewing range disabled");
    }
}

void GameInputSystem::checkZoomEnabled()
{
    LocalPlayer *player = client->getEnv().getLocalPlayer();
    if (player->getZoomFOV() < 0.001f || player->getFov().fov > 0.0f)
        m_GameInputSystem_ui->showTranslatedStatusText("Zoom currently disabled by GameInputSystem or mod");
}

void GameInputSystem::updateCameraDirection(CameraOrientation *cam, float dtime)
{
    auto *cur_control = device->getCursorControl();

    /* With CIrrDeviceSDL on Linux and Windows, enabling relative mouse mode
    somehow results in simulated mouse events being generated from touch events,
    although SDL_HINT_MOUSE_TOUCH_EVENTS and SDL_HINT_TOUCH_MOUSE_EVENTS are set to 0.
    Since Minetest has its own code to synthesize mouse events from touch events,
    this results in duplicated input. To avoid that, we don't enable relative
    mouse mode if we're in touchscreen mode. */
    if (cur_control)
        cur_control->setRelativeMode(!g_touchcontrols && !isMenuActive());

    if ((device->isWindowActive() && device->isWindowFocused()
            && !isMenuActive()) || input->isRandom()) {

        if (cur_control && !input->isRandom()) {
            // Mac OSX gets upset if this is set every frame
            if (cur_control->isVisible())
                cur_control->setVisible(false);
        }

        if (m_first_loop_after_window_activation && !g_touchcontrols) {
            m_first_loop_after_window_activation = false;

            input->setMousePos(driver->getScreenSize().Width / 2,
                driver->getScreenSize().Height / 2);
        } else {
            updateCameraOrientation(cam, dtime);
        }

    } else {
        // Mac OSX gets upset if this is set every frame
        if (cur_control && !cur_control->isVisible())
            cur_control->setVisible(true);

        m_first_loop_after_window_activation = true;
    }
    if (g_touchcontrols)
        m_first_loop_after_window_activation = true;
}

// Get the factor to multiply with sensitivity to get the same mouse/joystick
// responsiveness independently of FOV.
f32 GameInputSystem::getSensitivityScaleFactor() const
{
    f32 fov_y = client->getCamera()->getFovY();

    // Multiply by a constant such that it becomes 1.0 at 72 degree FOV and
    // 16:9 aspect ratio to minimize disruption of existing sensitivity
    // settings.
    return std::tan(fov_y / 2.0f) * 1.3763819f;
}

void GameInputSystem::updateCameraOrientation(CameraOrientation *cam, float dtime)
{
    if (g_touchcontrols) {
        cam->camera_yaw   += g_touchcontrols->getYawChange();
        cam->camera_pitch += g_touchcontrols->getPitchChange();
    } else {
        v2s32 center(driver->getScreenSize().Width / 2, driver->getScreenSize().Height / 2);
        v2s32 dist = input->getMousePos() - center;

        if (m_invert_mouse || camera->getCameraMode() == CAMERA_MODE_THIRD_FRONT) {
            dist.Y = -dist.Y;
        }

        f32 sens_scale = getSensitivityScaleFactor();
        cam->camera_yaw   -= dist.X * m_cache_mouse_sensitivity * sens_scale;
        cam->camera_pitch += dist.Y * m_cache_mouse_sensitivity * sens_scale;

        if (dist.X != 0 || dist.Y != 0)
            input->setMousePos(center.X, center.Y);
    }

    if (m_cache_enable_joysticks) {
        f32 sens_scale = getSensitivityScaleFactor();
        f32 c = m_cache_joystick_frustum_sensitivity * dtime * sens_scale;
        cam->camera_yaw -= input->joystick.getAxisWithoutDead(JA_FRUSTUM_HORIZONTAL) * c;
        cam->camera_pitch += input->joystick.getAxisWithoutDead(JA_FRUSTUM_VERTICAL) * c;
    }

    cam->camera_pitch = rangelim(cam->camera_pitch, -89.5, 89.5);
}


void GameInputSystem::updatePlayerControl(const CameraOrientation &cam)
{
    LocalPlayer *player = client->getEnv().getLocalPlayer();

    //TimeTaker tt("update player control", NULL, PRECISION_NANO);

    PlayerControl control(
        isKeyDown(KeyType::FORWARD),
        isKeyDown(KeyType::BACKWARD),
        isKeyDown(KeyType::LEFT),
        isKeyDown(KeyType::RIGHT),
        isKeyDown(KeyType::JUMP) || player->getAutojump(),
        isKeyDown(KeyType::AUX1),
        isKeyDown(KeyType::SNEAK),
        isKeyDown(KeyType::ZOOM),
        isKeyDown(KeyType::DIG),
        isKeyDown(KeyType::PLACE),
        cam.camera_pitch,
        cam.camera_yaw,
        input->getJoystickSpeed(),
        input->getJoystickDirection()
    );
    control.setMovementFromKeys();

    // autoforward if set: move at maximum speed
    if (player->getPlayerSettings().continuous_forward &&
            client->activeObjectsReceived() && !player->isDead()) {
        control.movement_speed = 1.0f;
        // sideways movement only
        float dx = std::sin(control.movement_direction);
        control.movement_direction = std::atan2(dx, 1.0f);
    }

    /* For touch, simulate holding down AUX1 (fast move) if the user has
     * the fast_move setting toggled on. If there is an aux1 key defined for
     * touch then its meaning is inverted (i.e. holding aux1 means walk and
     * not fast)
     */
    if (g_touchcontrols && m_touch_simulate_aux1) {
        control.aux1 = control.aux1 ^ true;
    }

    client->setPlayerControl(control);

    //tt.stop();
}
