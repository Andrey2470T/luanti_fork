#pragma once

#include <BasicIncludes.h>
#include "network/address.h"
#include "network/networkprotocol.h" // multiple enums
#include "network/peerhandler.h"
#include "gameparams.h"
#include <map>
#include <memory>
#include "serialization.h"
#include "util/string.h"

class NetworkPacket;
class Client;
class PointedThing;
class InventoryAction;
class ClientDynamicInfo;
class ModChannelMgr;

namespace con {
class IConnection;
}

/*
    Packet counter
*/

class PacketCounter
{
public:
    PacketCounter() = default;

    void add(u16 command)
    {
        auto n = m_packets.find(command);
        if (n == m_packets.end())
            m_packets[command] = 1;
        else
            n->second++;
    }

    void clear()
    {
        m_packets.clear();
    }

    u32 sum() const;
    void print(std::ostream &o) const;

private:
    // command, count
    std::map<u16, u32> m_packets;
};

class ClientPacketHandler : public con::PeerHandler
{
    Client *m_client;
    std::unique_ptr<con::IConnection> m_con;

    PacketCounter m_packetcounter;

    f32 m_packetcounter_timer = 0.0f;
    f32 m_connection_reinit_timer = 0.1f;
    f32 m_playerpos_send_timer = 0.0f;
    // An interval for generally sending object positions and stuff
    float m_recommended_send_interval = 0.1f;

    bool m_access_denied = false;
    bool m_access_denied_reconnect = false;
    std::string m_access_denied_reason = "";

    std::string m_address_name;
    ELoginRegister m_allow_login_or_register = ELoginRegister::Any;

    // Used version of the protocol with server
    // Values smaller than 25 only mean they are smaller than 25,
    // and aren't accurate. We simply just don't know, because
    // the server didn't send the version back then.
    // If 0, server init hasn't been received yet.
    u16 m_proto_ver = 0;

    // Server serialization version
    u8 m_server_ser_ver = SER_FMT_VER_INVALID;

    bool m_itemdef_received = false;
    bool m_nodedef_received = false;
    bool m_activeobjects_received = false;
    bool m_mods_loaded = false;
public:
    ClientPacketHandler(Client *client, ELoginRegister allow_login_or_register)
        : m_client(client), m_allow_login_or_register(allow_login_or_register)
    {}
    ~ClientPacketHandler();

    void connect(const Address &address, const std::string &address_name,
        bool is_local_server);

    /*
     * Command Handlers
     */

    void handleCommand(NetworkPacket* pkt);

    void handleCommand_Null(NetworkPacket* pkt) {};
    void handleCommand_Deprecated(NetworkPacket* pkt);
    void handleCommand_Hello(NetworkPacket* pkt);
    void handleCommand_AuthAccept(NetworkPacket* pkt);
    void handleCommand_AcceptSudoMode(NetworkPacket* pkt);
    void handleCommand_DenySudoMode(NetworkPacket* pkt);
    void handleCommand_AccessDenied(NetworkPacket* pkt);
    void handleCommand_RemoveNode(NetworkPacket* pkt);
    void handleCommand_AddNode(NetworkPacket* pkt);
    void handleCommand_NodemetaChanged(NetworkPacket *pkt);
    void handleCommand_BlockData(NetworkPacket* pkt);
    void handleCommand_Inventory(NetworkPacket* pkt);
    void handleCommand_TimeOfDay(NetworkPacket* pkt);
    void handleCommand_ChatMessage(NetworkPacket *pkt);
    void handleCommand_ActiveObjectRemoveAdd(NetworkPacket* pkt);
    void handleCommand_ActiveObjectMessages(NetworkPacket* pkt);
    void handleCommand_Movement(NetworkPacket* pkt);
    void handleCommand_Fov(NetworkPacket *pkt);
    void handleCommand_HP(NetworkPacket* pkt);
    void handleCommand_Breath(NetworkPacket* pkt);
    void handleCommand_MovePlayer(NetworkPacket* pkt);
    void handleCommand_MovePlayerRel(NetworkPacket* pkt);
    void handleCommand_DeathScreenLegacy(NetworkPacket* pkt);
    void handleCommand_AnnounceMedia(NetworkPacket* pkt);
    void handleCommand_Media(NetworkPacket* pkt);
    void handleCommand_NodeDef(NetworkPacket* pkt);
    void handleCommand_ItemDef(NetworkPacket* pkt);
    void handleCommand_PlaySound(NetworkPacket* pkt);
    void handleCommand_StopSound(NetworkPacket* pkt);
    void handleCommand_FadeSound(NetworkPacket *pkt);
    void handleCommand_Privileges(NetworkPacket* pkt);
    void handleCommand_InventoryFormSpec(NetworkPacket* pkt);
    void handleCommand_DetachedInventory(NetworkPacket* pkt);
    void handleCommand_ShowFormSpec(NetworkPacket* pkt);
    void handleCommand_SpawnParticle(NetworkPacket* pkt);
    void handleCommand_AddParticleSpawner(NetworkPacket* pkt);
    void handleCommand_DeleteParticleSpawner(NetworkPacket* pkt);
    void handleCommand_HudAdd(NetworkPacket* pkt);
    void handleCommand_HudRemove(NetworkPacket* pkt);
    void handleCommand_HudChange(NetworkPacket* pkt);
    void handleCommand_HudSetFlags(NetworkPacket* pkt);
    void handleCommand_HudSetParam(NetworkPacket* pkt);
    void handleCommand_HudSetSky(NetworkPacket* pkt);
    void handleCommand_HudSetSun(NetworkPacket* pkt);
    void handleCommand_HudSetMoon(NetworkPacket* pkt);
    void handleCommand_HudSetStars(NetworkPacket* pkt);
    void handleCommand_CloudParams(NetworkPacket* pkt);
    void handleCommand_OverrideDayNightRatio(NetworkPacket* pkt);
    void handleCommand_LocalPlayerAnimations(NetworkPacket* pkt);
    void handleCommand_EyeOffset(NetworkPacket* pkt);
    void handleCommand_UpdatePlayerList(NetworkPacket* pkt);
    void handleCommand_ModChannelMsg(NetworkPacket *pkt);
    void handleCommand_ModChannelSignal(NetworkPacket *pkt);
    void handleCommand_SrpBytesSandB(NetworkPacket *pkt);
    void handleCommand_FormspecPrepend(NetworkPacket *pkt);
    void handleCommand_CSMRestrictionFlags(NetworkPacket *pkt);
    void handleCommand_PlayerSpeed(NetworkPacket *pkt);
    void handleCommand_MediaPush(NetworkPacket *pkt);
    void handleCommand_MinimapModes(NetworkPacket *pkt);
    void handleCommand_SetLighting(NetworkPacket *pkt);

    void processData(NetworkPacket *pkt);

    void send(NetworkPacket* pkt);

    void interact(f32 range, InteractAction action, const PointedThing &pointed);

    void sendNodemetaFields(v3s16 p, const std::string &formname,
        const StringMap &fields);
    void sendInventoryFields(const std::string &formname,
        const StringMap &fields);
    void sendInventoryAction(InventoryAction *a);
    void sendChatMessage(const std::wstring &message);
    void sendDamage(u16 damage);
    void sendRespawnLegacy();
    void sendReady();
    void sendHaveMedia(const std::vector<u32> &tokens);
    void sendUpdateClientInfo(const ClientDynamicInfo &info);
    // Send the item number 'item' as player item to the server
    void sendPlayerItem(u16 item);

    bool accessDenied() const { return m_access_denied; }

    bool reconnectRequested() const { return m_access_denied_reconnect; }

    void setFatalError(const std::string &reason)
    {
        m_access_denied = true;
        m_access_denied_reason = reason;
    }
    inline void setFatalError(const LuaError &e)
    {
        setFatalError(std::string("Lua: ") + e.what());
    }

    bool itemdefReceived() const
    { return m_itemdef_received; }
    bool nodedefReceived() const
    { return m_nodedef_received; }
    bool activeObjectsReceived() const
    { return m_activeobjects_received; }

    // Renaming accessDeniedReason to better name could be good as it's used to
    // disconnect client when CSM failed.
    const std::string &accessDeniedReason() const { return m_access_denied_reason; }

    // IP and port we're connected to
    const Address getServerAddress();

    float getRTT();
    float getCurRate();

    // Hostname of the connected server (but can also be a numerical IP)
    const std::string &getAddressName() const
    {
        return m_address_name;
    }

    u16 getProtoVersion() const
    { return m_proto_ver; }

    // has the server ever replied to us, used for connection retry/fallback
    bool hasServerReplied() const {
        return getProtoVersion() != 0; // (set in TOCLIENT_HELLO)
    }

    void printPacketCounter(f32 dtime);

    void peerAdded(con::IPeer *peer) override;
    void deletingPeer(con::IPeer *peer, bool timeout) override;

    void request_media(const std::vector<std::string> &file_requests);

    void receiveAll();

    void sendPlayerPos(f32 range, f32 dtime);

    void sendInit(f32 dtime);
    void sendAuthFirstSRP(std::string verifier, std::string salt, bool pwd_empty);
    void sendAuthSRPBytesA(char *bytes_A, size_t len_A, u8 based_on);
    void sendDeletedBlocks(std::vector<v3s16> &blocks);
    void sendGotBlocks(const std::vector<v3s16> &blocks);
    void sendRemovedSounds(const std::vector<s32> &soundList);

    bool canSendChatMessage() const;

    bool joinModChannel(ModChannelMgr *mochannel, const std::string &channel);
    bool leaveModChannel(ModChannelMgr *mochannel, const std::string &channel);
    bool sendModChannelMessage(ModChannelMgr *mochannel, const std::string &channel, const std::string &message);
};
