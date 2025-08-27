#include "clientevent.h"
#include <queue>

class Client;
class ClientEventHandler;

struct ClientEventCallback
{
    void (ClientEventHandler::*handler)(ClientEvent *);
};

class ClientEventHandler
{
    Client *client;

    std::queue<ClientEvent *> clientEventQueue;

    static const ClientEventHandler clientEventHandler[CLIENTEVENT_MAX];
public:
    ClientEventHandler(Client *_client);

    bool hasClientEvents() const { return !clientEventQueue.empty(); }

    // Get event from queue. If queue is empty, it triggers an assertion failure.
    ClientEvent * getClientEvent();

    void pushToEventQueue(ClientEvent *event);

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
