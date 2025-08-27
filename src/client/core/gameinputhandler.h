#include "client.h"

class InputHandler;
class MtEventReceiver;

class GameInputSystem
{
    Client *client;
    std::unique_ptr<MtEventReceiver> receiver;
    std::unique_ptr<InputHandler> input;
public:
    GameInputSystem(Client *_client, bool random_input = false);

    void processUserInput(f32 dtime);
    void processKeyInput();
    void processItemSelection(u16 *new_playeritem);
    bool shouldShowTouchControls();

    void dropSelectedItem(bool single_item = false);
    void openConsole(f32 scale, const wchar_t *line=NULL);
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

    void updateCameraDirection(f32 dtime);
    void updateCameraOrientation(f32 dtime);
    void updatePlayerControl();
    void updatePauseState();
    void step(f32 dtime);
    void processClientEvents();
    void updateCamera(f32 dtime);
    void updateSound(f32 dtime);
    void processPlayerInteraction(f32 dtime, bool show_hud);
};
