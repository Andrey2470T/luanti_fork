#include "clientevent.h"
#include <queue>

class Client;
class ClientEventHandler;
class RenderSystem;
class ResourceCache;
class LocalPlayer;
class GameFormSpec;
class Hud;
class Sky;

typedef std::array<void (ClientEventHandler::*)(ClientEvent*), CLIENTEVENT_MAX> ClientEventTable;

class ClientEventHandler
{
    Client *client;
    RenderSystem *rndsys;
    ResourceCache *cache;
    LocalPlayer *player;
    GameFormSpec *formspec;
    Hud *hud;
    Sky *sky;

    std::queue<ClientEvent *> clientEventQueue;

    static const ClientEventTable clientEventTable;

    std::unordered_map<u32, u32> hud_server_to_client;
public:
    ClientEventHandler(Client *_client);

    // Get event from queue. If queue is empty, it triggers an assertion failure.
    ClientEvent * getClientEvent();

    void pushToEventQueue(ClientEvent *event);

    void processEvents();

    void handleClientEvent_None(ClientEvent *event);
    void handleClientEvent_PlayerDamage(ClientEvent *event);
    void handleClientEvent_PlayerForceMove(ClientEvent *event);
    void handleClientEvent_DeathscreenLegacy(ClientEvent *event);
    void handleClientEvent_ShowFormSpec(ClientEvent *event);
    void handleClientEvent_ShowLocalFormSpec(ClientEvent *event);
    void handleClientEvent_HandleParticleEvent(ClientEvent *event);
    void handleClientEvent_HudAdd(ClientEvent *event);
    void handleClientEvent_HudRemove(ClientEvent *event);
    void handleClientEvent_HudChange(ClientEvent *event);
    void handleClientEvent_SetSky(ClientEvent *event);
    void handleClientEvent_SetSun(ClientEvent *event);
    void handleClientEvent_SetMoon(ClientEvent *event);
    void handleClientEvent_SetStars(ClientEvent *event);
    void handleClientEvent_OverrideDayNigthRatio(ClientEvent *event);
    void handleClientEvent_CloudParams(ClientEvent *event);
};
