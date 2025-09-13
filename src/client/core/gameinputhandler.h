#include <BasicIncludes.h>
#include <Core/MainWindow.h>
#include "gui/IGUIEnvironment.h"

class Client;
class InputHandler;
class MyEventReceiver;
class GUIChatConsole;
class LocalPlayer;
class GameUI;
class PlayerCamera;
class Sky;
class QuicktuneShortcutter;

class GameInputSystem
{
    Client *client;
    InputHandler *input;
    MyEventReceiver *receiver;
    gui::IGUIEnvironment *guienv;
    core::MainWindow *wnd;
    GUIChatConsole *chat_console;
    LocalPlayer *player;
    GameUI *gameui;
    PlayerCamera *camera;
    Sky *sky;

    std::unique_ptr<QuicktuneShortcutter> quicktune;

    f32 jump_timer_up;          // from key up until key down
    f32 jump_timer_down;        // since last key down
    f32 jump_timer_down_before; // from key down until key down again
    bool reset_jump_timer;

    bool game_focused = false;

    /*
     * TODO: Local caching of settings is not optimal and should at some stage
     *       be updated to use a global settings object for getting thse values
     *       (as opposed to the this local caching). This can be addressed in
     *       a later release.
     */
    bool cache_doubletap_jump;
    bool cache_enable_joysticks;
    bool cache_enable_noclip;
    bool cache_enable_free_move;
    f32  cache_mouse_sensitivity;
    f32  cache_joystick_frustum_sensitivity;
    f32  cache_cam_smoothing;

    bool invert_mouse;
    bool enable_hotbar_mouse_wheel;
    bool invert_hotbar_mouse_wheel;

    bool touch_simulate_aux1 = false;

    bool first_loop_after_window_activation = true;

public:
    GameInputSystem(Client *_client);

    void processUserInput(f32 dtime);
    void processKeyInput();
    void processItemSelection(u16 *new_playeritem);
    bool shouldShowTouchControls();

    void toggleFreeMove();
    void toggleFreeMoveAlt();
    void togglePitchMove();
    void toggleFast();
    void toggleNoClip();
    void toggleCinematic();
    void toggleBlockBounds();
    void toggleAutoforward();

    void toggleMinimap(bool shift_pressed);
    void toggleFog();
    void toggleDebug();
    void toggleUpdateCamera();

    void increaseViewRange();
    void decreaseViewRange();
    void toggleFullViewRange();
    void checkZoomEnabled();

    void updateCameraMode();
    void updateCameraDirection(f32 dtime);
    void updatePlayerControl();

    static void settingChangedCallback(const std::string &setting_name, void *data);
    void readSettings();
};
