#include "gameinputhandler.h"
#include "client.h"
#include "client/core/chatmessanger.h"
#include "client/event/inputhandler.h"
#include "client/event/eventreceiver.h"
#include "client/player/interaction.h"
#include "client/player/selection.h"
#include "client/render/drawlist.h"
#include "client/render/rendersystem.h"
#include <Core/MainWindow.h>
#include "client/render/sky.h"
#include "client/ui/gameformspec.h"
#include "client/ui/gameui.h"
#include "client/ui/hud.h"
#include "client/ui/minimap.h"
#include "gui/guiChatConsole.h"
#include "gui/mainmenumanager.h"
#include "gui/touchcontrols.h"
#include "client/player/localplayer.h"
#include "client/ao/renderCAO.h"
#include "hud.h"
#include "settings.h"
#include "util/quicktune_shortcutter.h"
#include "client/network/packethandler.h"
#include "util/numeric.h"
#include "gettext.h"

GameInputSystem::GameInputSystem(Client *_client)
    : client(_client), input(client->getInputHandler()),
      receiver(input->getReceiver()), guienv(client->getRenderSystem()->getGUIEnvironment()),
      wnd(client->getRenderSystem()->getWindow()), chat_console(client->getChatMessanger()->getChatConsole()),
      player(client->getEnv().getLocalPlayer()), gameui(client->getRenderSystem()->getGameUI()),
      camera(player->getCamera()), sky(client->getRenderSystem()->getSky()),
      quicktune(std::make_unique<QuicktuneShortcutter>())
{
    g_settings->registerChangedCallback("doubletap_jump",
        &settingChangedCallback, this);
    g_settings->registerChangedCallback("enable_joysticks",
        &settingChangedCallback, this);
    g_settings->registerChangedCallback("noclip",
        &settingChangedCallback, this);
    g_settings->registerChangedCallback("free_move",
        &settingChangedCallback, this);
    g_settings->registerChangedCallback("mouse_sensitivity",
        &settingChangedCallback, this);
    g_settings->registerChangedCallback("joystick_frustum_sensitivity",
        &settingChangedCallback, this);
    g_settings->registerChangedCallback("cinematic",
        &settingChangedCallback, this);
    g_settings->registerChangedCallback("cinematic_camera_smoothing",
        &settingChangedCallback, this);
    g_settings->registerChangedCallback("camera_smoothing",
        &settingChangedCallback, this);
    g_settings->registerChangedCallback("invert_mouse",
        &settingChangedCallback, this);
    g_settings->registerChangedCallback("enable_hotbar_mouse_wheel",
        &settingChangedCallback, this);
    g_settings->registerChangedCallback("invert_hotbar_mouse_wheel",
        &settingChangedCallback, this);
    g_settings->registerChangedCallback("fast_move",
        &settingChangedCallback, this);
}

void GameInputSystem::processUserInput(f32 dtime)
{
    bool desired = client->shouldShowTouchControls();
    if (desired && !g_touchcontrols) {
        //g_touchcontrols = new TouchControls(device, texture_src);

    } else if (!desired && g_touchcontrols) {
        delete g_touchcontrols;
        g_touchcontrols = nullptr;
    }

    // Reset input if window not active or some menu is active
    if (!wnd->isActive() || isMenuActive() || guienv->hasFocus(chat_console)) {
        if (game_focused) {
            game_focused = false;
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
            g_touchcontrols->step(dtime, player->getCamera());
        }

        game_focused = true;
    }

    if (!guienv->hasFocus(chat_console) && chat_console->isOpen()
        && !chat_console->isMyChild(guienv->getFocus()))
    {
        chat_console->closeConsoleAtOnce();
    }

    // Input handler step() (used by the random input generator)
    input->step(dtime);

#ifdef __ANDROID__
    if (!m_GameInputSystem_formspec.handleAndroidUIInput())
        handleAndroidChatInput();
#endif

    // Increase timer for double tap of "keymap_jump"
    if (cache_doubletap_jump && jump_timer_up <= 0.2f)
        jump_timer_up += dtime;
    if (cache_doubletap_jump && jump_timer_down <= 0.4f)
        jump_timer_down += dtime;

    processKeyInput();
    processItemSelection(&client->getEnv().new_playeritem);
}


void GameInputSystem::processKeyInput()
{
    auto formspec = client->getRenderSystem()->getGameFormSpec();
    auto chat_msgr = client->getChatMessanger();

    if (input->wasKeyDown(KeyType::DROP)) {
        player->dropSelectedItem(input->isKeyDown(KeyType::SNEAK));
    } else if (input->wasKeyDown(KeyType::AUTOFORWARD)) {
        toggleAutoforward();
    } else if (input->wasKeyDown(KeyType::BACKWARD)) {
        if (g_settings->getBool("continuous_forward"))
            toggleAutoforward();
    } else if (input->wasKeyDown(KeyType::INVENTORY)) {
        formspec->showPlayerInventory();
    } else if (input->cancelPressed()) {
#ifdef __ANDROID__
        chat_msgr->android_chat_open = false;
#endif
        if (!chat_console->isOpenInhibited()) {
            formspec->showPauseMenu();
        }
    } else if (input->wasKeyDown(KeyType::CHAT)) {
        chat_msgr->openConsole(0.2, L"");
    } else if (input->wasKeyDown(KeyType::CMD)) {
        chat_msgr->openConsole(0.2, L"/");
    } else if (input->wasKeyDown(KeyType::CMD_LOCAL)) {
        if (client->modsLoaded())
            chat_msgr->openConsole(0.2, L".");
        else
            gameui->showTranslatedStatusText("Client side scripting is disabled");
    } else if (input->wasKeyDown(KeyType::CONSOLE)) {
        chat_msgr->openConsole(std::clamp(g_settings->getFloat("console_height"), 0.1f, 1.0f));
    } else if (input->wasKeyDown(KeyType::FREEMOVE)) {
        toggleFreeMove();
    } else if (input->wasKeyDown(KeyType::JUMP)) {
        toggleFreeMoveAlt();
    } else if (input->wasKeyDown(KeyType::PITCHMOVE)) {
        togglePitchMove();
    } else if (input->wasKeyDown(KeyType::FASTMOVE)) {
        toggleFast();
    } else if (input->wasKeyDown(KeyType::NOCLIP)) {
        toggleNoClip();
#if USE_SOUND
    } else if (input->wasKeyDown(KeyType::MUTE)) {
        bool new_mute_sound = !g_settings->getBool("mute_sound");
        g_settings->setBool("mute_sound", new_mute_sound);
        if (new_mute_sound)
            gameui->showTranslatedStatusText("Sound muted");
        else
            gameui->showTranslatedStatusText("Sound unmuted");
    } else if (input->wasKeyDown(KeyType::INC_VOLUME)) {
        float new_volume = g_settings->getFloat("sound_volume", 0.0f, 0.9f) + 0.1f;
        g_settings->setFloat("sound_volume", new_volume);
        std::wstring msg = fwgettext("Volume changed to %d%%", myround(new_volume * 100));
        gameui->showStatusText(msg);
    } else if (input->wasKeyDown(KeyType::DEC_VOLUME)) {
        float new_volume = g_settings->getFloat("sound_volume", 0.1f, 1.0f) - 0.1f;
        g_settings->setFloat("sound_volume", new_volume);
        std::wstring msg = fwgettext("Volume changed to %d%%", myround(new_volume * 100));
        gameui->showStatusText(msg);
#else
    } else if (wasKeyDown(KeyType::MUTE) || wasKeyDown(KeyType::INC_VOLUME)
            || wasKeyDown(KeyType::DEC_VOLUME)) {
        gameui->showTranslatedStatusText("Sound system is not supported on this build");
#endif
    } else if (input->wasKeyDown(KeyType::CINEMATIC)) {
        toggleCinematic();
    } else if (input->wasKeyPressed(KeyType::SCREENSHOT)) {
        //client->makeScreenshot();
    } else if (input->wasKeyPressed(KeyType::TOGGLE_BLOCK_BOUNDS)) {
        toggleBlockBounds();
    } else if (input->wasKeyPressed(KeyType::TOGGLE_HUD)) {
        gameui->toggleHud();
    } else if (input->wasKeyPressed(KeyType::MINIMAP)) {
        toggleMinimap(input->isKeyDown(KeyType::SNEAK));
    } else if (input->wasKeyPressed(KeyType::TOGGLE_CHAT)) {
        gameui->toggleChat(client);
    } else if (input->wasKeyPressed(KeyType::TOGGLE_FOG)) {
        toggleFog();
    } else if (input->wasKeyDown(KeyType::TOGGLE_UPDATE_CAMERA)) {
        toggleUpdateCamera();
    } else if (input->wasKeyPressed(KeyType::TOGGLE_DEBUG)) {
        toggleDebug();
    } else if (input->wasKeyPressed(KeyType::TOGGLE_PROFILER)) {
        gameui->toggleProfiler();
    } else if (input->wasKeyDown(KeyType::INCREASE_VIEWING_RANGE)) {
        increaseViewRange();
    } else if (input->wasKeyDown(KeyType::DECREASE_VIEWING_RANGE)) {
        decreaseViewRange();
    } else if (input->wasKeyPressed(KeyType::RANGESELECT)) {
        toggleFullViewRange();
    } else if (input->wasKeyDown(KeyType::ZOOM)) {
        checkZoomEnabled();
    } else if (input->wasKeyDown(KeyType::QUICKTUNE_NEXT)) {
        quicktune->next();
    } else if (input->wasKeyDown(KeyType::QUICKTUNE_PREV)) {
        quicktune->prev();
    } else if (input->wasKeyDown(KeyType::QUICKTUNE_INC)) {
        quicktune->inc();
    } else if (input->wasKeyDown(KeyType::QUICKTUNE_DEC)) {
        quicktune->dec();
    }

    if (!input->isKeyDown(KeyType::JUMP) && reset_jump_timer) {
        reset_jump_timer = false;
        jump_timer_up = 0.0f;
    }

    if (quicktune->hasMessage()) {
        gameui->showStatusText(utf8_to_wide(quicktune->getMessage()));
    }
}

void GameInputSystem::processItemSelection(u16 *new_playeritem)
{
    *new_playeritem = player->getWieldIndex();
    u16 max_item = player->getMaxHotbarItemcount();
    if (max_item == 0)
        return;
    max_item -= 1;

    /* Item selection using mouse wheel
     */
    s32 wheel = input->getMouseWheel();
    if (!enable_hotbar_mouse_wheel)
        wheel = 0;
    if (invert_hotbar_mouse_wheel)
        wheel *= -1;

    s32 dir = wheel;

    if (input->wasKeyDown(KeyType::HOTBAR_NEXT))
        dir = -1;

    if (input->wasKeyDown(KeyType::HOTBAR_PREV))
        dir = 1;

    if (dir < 0)
        *new_playeritem = *new_playeritem < max_item ? *new_playeritem + 1 : 0;
    else if (dir > 0)
        *new_playeritem = *new_playeritem > 0 ? *new_playeritem - 1 : max_item;
    // else dir == 0

    /* Item selection using hotbar slot keys
     */
    for (u16 i = 0; i <= max_item; i++) {
        if (input->wasKeyDown((GameKeyType)(KeyType::SLOT_1 + i))) {
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

void GameInputSystem::toggleFreeMove()
{
    bool free_move = !g_settings->getBool("free_move");
    g_settings->set("free_move", bool_to_cstr(free_move));

    if (free_move) {
        if (player->checkPrivilege("fly")) {
            gameui->showTranslatedStatusText("Fly mode enabled");
        } else {
            gameui->showTranslatedStatusText("Fly mode enabled (note: no 'fly' privilege)");
        }
    } else {
        gameui->showTranslatedStatusText("Fly mode disabled");
    }
}

void GameInputSystem::toggleFreeMoveAlt()
{
    if (!reset_jump_timer) {
        jump_timer_down_before = jump_timer_down;
        jump_timer_down = 0.0f;
    }

    // key down (0.2 s max.), then key up (0.2 s max.), then key down
    if (cache_doubletap_jump && jump_timer_up < 0.2f &&
            jump_timer_down_before < 0.4f) // 0.2 + 0.2
        toggleFreeMove();

    reset_jump_timer = true;
}


void GameInputSystem::togglePitchMove()
{
    bool pitch_move = !g_settings->getBool("pitch_move");
    g_settings->set("pitch_move", bool_to_cstr(pitch_move));

    if (pitch_move) {
        gameui->showTranslatedStatusText("Pitch move mode enabled");
    } else {
        gameui->showTranslatedStatusText("Pitch move mode disabled");
    }
}


void GameInputSystem::toggleFast()
{
    bool fast_move = !g_settings->getBool("fast_move");
    bool has_fast_privs = player->checkPrivilege("fast");
    g_settings->set("fast_move", bool_to_cstr(fast_move));

    if (fast_move) {
        if (has_fast_privs) {
            gameui->showTranslatedStatusText("Fast mode enabled");
        } else {
            gameui->showTranslatedStatusText("Fast mode enabled (note: no 'fast' privilege)");
        }
    } else {
        gameui->showTranslatedStatusText("Fast mode disabled");
    }

    touch_simulate_aux1 = fast_move && has_fast_privs;
}


void GameInputSystem::toggleNoClip()
{
    bool noclip = !g_settings->getBool("noclip");
    g_settings->set("noclip", bool_to_cstr(noclip));

    if (noclip) {
        if (player->checkPrivilege("noclip")) {
            gameui->showTranslatedStatusText("Noclip mode enabled");
        } else {
            gameui->showTranslatedStatusText("Noclip mode enabled (note: no 'noclip' privilege)");
        }
    } else {
        gameui->showTranslatedStatusText("Noclip mode disabled");
    }
}

void GameInputSystem::toggleCinematic()
{
    bool cinematic = !g_settings->getBool("cinematic");
    g_settings->set("cinematic", bool_to_cstr(cinematic));

    if (cinematic)
        gameui->showTranslatedStatusText("Cinematic mode enabled");
    else
        gameui->showTranslatedStatusText("Cinematic mode disabled");
}

void GameInputSystem::toggleBlockBounds()
{
    if (!(player->checkPrivilege("debug") || (player->hud_flags & HUD_FLAG_BASIC_DEBUG))) {
        gameui->showTranslatedStatusText("Can't show block bounds (disabled by GameInputSystem or mod)");
        return;
    }

    auto drawlist = client->getRenderSystem()->getDrawList();
    auto blockbounds = drawlist->getBlockBounds();
    BlockBounds::Mode newmode = blockbounds->toggle(client, drawlist);
    switch (newmode) {
        case BlockBounds::Mode::BLOCK_BOUNDS_OFF:
            gameui->showTranslatedStatusText("Block bounds hidden");
            break;
        case BlockBounds::Mode::BLOCK_BOUNDS_CURRENT:
            gameui->showTranslatedStatusText("Block bounds shown for current block");
            break;
        case BlockBounds::Mode::BLOCK_BOUNDS_NEAR:
            gameui->showTranslatedStatusText("Block bounds shown for nearby blocks");
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
        gameui->showTranslatedStatusText("Automatic forward enabled");
    else
        gameui->showTranslatedStatusText("Automatic forward disabled");
}

void GameInputSystem::toggleMinimap(bool shift_pressed)
{
    auto minimap = client->getRenderSystem()->getDefaultMinimap();

    if (!minimap || !(gameui->getFlags() & GUIF_SHOW_HUD) || !g_settings->getBool("enable_minimap"))
        return;

    if (shift_pressed)
        minimap->toggleMinimapShape();
    else
        minimap->nextMode();

    // TODO: When legacy minimap is deprecated, keep only HUD minimap stuff here

    // Not so satisying code to keep compatibility with old fixed mode system
    // -->
    u32 hud_flags = client->getEnv().getLocalPlayer()->hud_flags;

    if (hud_flags & HUD_FLAG_MINIMAP_VISIBLE) {
    // If radar is disabled, try to find a non radar mode or fall back to 0
        if (!(hud_flags & HUD_FLAG_MINIMAP_RADAR_VISIBLE))
            while (minimap->getModeIndex() &&
                    minimap->getModeDef().type == MINIMAP_TYPE_RADAR)
                minimap->nextMode();
    }
    // <--
    // End of 'not so satifying code'
    if (gameui->getHud()->hasElementOfType(HUD_ELEM_MINIMAP))
        gameui->showStatusText(utf8_to_wide(minimap->getModeDef().label));
    else
        gameui->showTranslatedStatusText("Minimap currently disabled by GameInputSystem or mod");
}

void GameInputSystem::toggleFog()
{
    bool flag = !g_settings->getBool("enable_fog");
    g_settings->setBool("enable_fog", flag);
    bool allowed = sky->getFogDistance() < 0 || player->checkPrivilege("debug");
    if (!allowed)
        gameui->showTranslatedStatusText("Fog enabled by GameInputSystem or mod");
    else if (flag)
        gameui->showTranslatedStatusText("Fog enabled");
    else
        gameui->showTranslatedStatusText("Fog disabled");
}


void GameInputSystem::toggleDebug()
{
    u8 state = gameui->toggleDebug(player);
    client->getRenderSystem()->getDrawList()->getDrawControl().show_wireframe = state == 3;

    if (state == 1) {
        gameui->showTranslatedStatusText("Debug info shown");
    } else if (state == 2) {
        gameui->showTranslatedStatusText("Profiler graph shown");
    } else if (state == 3) {
        if (wnd->getOpenGLVersion().Type == core::OGL_TYPE_ES)
            gameui->showTranslatedStatusText("Wireframe not supported by video driver");
        else
            gameui->showTranslatedStatusText("Wireframe shown");
    } else if (state == 4) {
        gameui->showTranslatedStatusText("Bounding boxes shown");
    } else {
        gameui->showTranslatedStatusText("All debug info hidden");
    }
}


void GameInputSystem::toggleUpdateCamera()
{
    camera->disable_update = player->checkPrivilege("debug") ? !camera->disable_update : false;
    if (camera->disable_update)
        gameui->showTranslatedStatusText("Camera update disabled");
    else
        gameui->showTranslatedStatusText("Camera update enabled");
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
        gameui->showStatusText(msg);
    } else {
        std::wstring msg = server_limit >= 0 && range_new > server_limit ?
                fwgettext("Viewing range changed to %d, but limited to %d by GameInputSystem or mod", range_new, server_limit) :
                fwgettext("Viewing range changed to %d", range_new);
        gameui->showStatusText(msg);
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
        gameui->showStatusText(msg);
    } else {
        std::wstring msg = server_limit >= 0 && range_new > server_limit ?
                fwgettext("Viewing range changed to %d, but limited to %d by GameInputSystem or mod", range_new, server_limit) :
                fwgettext("Viewing range changed to %d", range_new);
        gameui->showStatusText(msg);
    }
    g_settings->set("viewing_range", itos(range_new));
}


void GameInputSystem::toggleFullViewRange()
{
    auto draw_control = client->getRenderSystem()->getDrawList()->getDrawControl();
    draw_control.range_all = !draw_control.range_all;
    if (draw_control.range_all) {
        if (sky->getFogDistance() >= 0) {
            gameui->showTranslatedStatusText("Unlimited viewing range enabled, but forbidden by GameInputSystem or mod");
        } else {
            gameui->showTranslatedStatusText("Unlimited viewing range enabled");
        }
    } else {
        gameui->showTranslatedStatusText("Unlimited viewing range disabled");
    }
}

void GameInputSystem::checkZoomEnabled()
{
    if (player->getZoomFOV() < 0.001f || player->getFov().fov > 0.0f)
        gameui->showTranslatedStatusText("Zoom currently disabled by GameInputSystem or mod");
}

void GameInputSystem::updateCameraMode()
{
    if (input->wasKeyPressed(KeyType::CAMERA_MODE)) {
        RenderCAO *playercao = player->getCAO();

        // If playercao not loaded, don't change camera
        if (!playercao)
            return;

        camera->toggleCameraMode();

        if (g_touchcontrols)
            g_touchcontrols->setUseCrosshair(!player->getInteraction()->isTouchCrosshairDisabled());

        // Make the player visible depending on camera mode.
        playercao->updateMeshCulling();
        playercao->setChildrenVisible(camera->getCameraMode() > CAMERA_MODE_FIRST);
    }
}

void GameInputSystem::updateCameraDirection(float dtime)
{
    auto cur_control = wnd->getCursorControl();

    /* With CIrrDeviceSDL on Linux and Windows, enabling relative mouse mode
    somehow results in simulated mouse events being generated from touch events,
    although SDL_HINT_MOUSE_TOUCH_EVENTS and SDL_HINT_TOUCH_MOUSE_EVENTS are set to 0.
    Since Minetest has its own code to synthesize mouse events from touch events,
    this results in duplicated input. To avoid that, we don't enable relative
    mouse mode if we're in touchscreen mode. */

    //cur_control.setRelativeMode(!g_touchcontrols && !isMenuActive());

    if ((wnd->isActive() && wnd->isFocused()
            && !isMenuActive()) || input->isRandom()) {

        if (!input->isRandom()) {
            // Mac OSX gets upset if this is set every frame
            if (cur_control.isVisible())
                cur_control.setVisible(false);
        }

        if (first_loop_after_window_activation && !g_touchcontrols) {
            first_loop_after_window_activation = false;

            input->setMousePos(wnd->getWindowSize().X / 2, wnd->getWindowSize().Y / 2);
        } else {
            camera->updateOrientation(invert_mouse, cache_mouse_sensitivity, cache_enable_joysticks,
                cache_joystick_frustum_sensitivity, cache_cam_smoothing, dtime);
        }

    } else {
        // Mac OSX gets upset if this is set every frame
        if (!cur_control.isVisible())
            cur_control.setVisible(true);

        first_loop_after_window_activation = true;
    }
    if (g_touchcontrols)
        first_loop_after_window_activation = true;
}

void GameInputSystem::updatePlayerControl()
{
    //TimeTaker tt("update player control", NULL, PRECISION_NANO);

    v2f cam_orient = player->getCamera()->getOrientation();
    PlayerControl control(
        input->isKeyDown(KeyType::FORWARD),
        input->isKeyDown(KeyType::BACKWARD),
        input->isKeyDown(KeyType::LEFT),
        input->isKeyDown(KeyType::RIGHT),
        input->isKeyDown(KeyType::JUMP) || player->getAutojump(),
        input->isKeyDown(KeyType::AUX1),
        input->isKeyDown(KeyType::SNEAK),
        input->isKeyDown(KeyType::ZOOM),
        input->isKeyDown(KeyType::DIG),
        input->isKeyDown(KeyType::PLACE),
        cam_orient.X,
        cam_orient.Y,
        input->getJoystickSpeed(),
        input->getJoystickDirection()
    );
    control.setMovementFromKeys();

    // autoforward if set: move at maximum speed
    if (player->getPlayerSettings().continuous_forward &&
            client->getPacketHandler()->activeObjectsReceived() && !player->isDead()) {
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
    if (g_touchcontrols && touch_simulate_aux1) {
        control.aux1 = control.aux1 ^ true;
    }

    player->setPlayerControl(control);

    //tt.stop();
}

void GameInputSystem::settingChangedCallback(const std::string &setting_name, void *data)
{
    ((GameInputSystem *)data)->readSettings();
}

void GameInputSystem::readSettings()
{
    cache_doubletap_jump               = g_settings->getBool("doubletap_jump");
    cache_enable_joysticks             = g_settings->getBool("enable_joysticks");
    cache_mouse_sensitivity            = g_settings->getFloat("mouse_sensitivity", 0.001f, 10.0f);
    cache_joystick_frustum_sensitivity = std::max(g_settings->getFloat("joystick_frustum_sensitivity"), 0.001f);

    cache_cam_smoothing = 0;
    if (g_settings->getBool("cinematic"))
        cache_cam_smoothing = 1 - g_settings->getFloat("cinematic_camera_smoothing");
    else
        cache_cam_smoothing = 1 - g_settings->getFloat("camera_smoothing");

    cache_cam_smoothing = rangelim(cache_cam_smoothing, 0.01f, 1.0f);
    cache_mouse_sensitivity = rangelim(cache_mouse_sensitivity, 0.001, 100.0);

    invert_mouse = g_settings->getBool("invert_mouse");
    enable_hotbar_mouse_wheel = g_settings->getBool("enable_hotbar_mouse_wheel");
    invert_hotbar_mouse_wheel = g_settings->getBool("invert_hotbar_mouse_wheel");

    touch_simulate_aux1 = g_settings->getBool("fast_move")
        && client->getEnv().getLocalPlayer()->checkPrivilege("fast");
}
