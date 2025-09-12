#include "clientevent.h"
#include <queue>

class Client;
class ClientEventHandler;
class RenderSystem;
class LocalPlayer;

typedef std::array<void (ClientEventHandler::*)(ClientEvent*), CLIENTEVENT_MAX> ClientEventTable;

class ClientEventHandler
{
    Client *client;
    RenderSystem *rndsys;
    LocalPlayer *player;

    std::queue<ClientEvent *> clientEventQueue;

    static const ClientEventTable clientEventTable;

public:
    ClientEventHandler(Client *_client);

    bool hasClientEvents() const { return !clientEventQueue.empty(); }

    // Get event from queue. If queue is empty, it triggers an assertion failure.
    ClientEvent * getClientEvent();

    void pushToEventQueue(ClientEvent *event);

    void processClientEvents();

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
