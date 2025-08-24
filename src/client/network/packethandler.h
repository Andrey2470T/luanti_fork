#pragma once

#include <BasicIncludes.h>
#include "network/address.h"
#include "network/networkprotocol.h" // multiple enums
#include "network/peerhandler.h"
#include <map>
#include <memory>

class NetworkPacket;

namespace con {
class IConnection;
}

enum LocalClientState {
    LC_Created,
    LC_Init,
    LC_Ready
};

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
    std::unique_ptr<con::IConnection> m_con;

    PacketCounter m_packetcounter;

    f32 m_packetcounter_timer = 0.0f;
    f32 m_connection_reinit_timer = 0.1f;
    f32 m_avg_rtt_timer = 0.0f;
    f32 m_playerpos_send_timer = 0.0f;

    bool m_access_denied = false;
    bool m_access_denied_reconnect = false;
    std::string m_access_denied_reason = "";

    std::string m_address_name;
    ELoginRegister m_allow_login_or_register = ELoginRegister::Any;

    // own state
    LocalClientState m_state;
public:
    ClientPacketHandler();

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

    void ProcessData(NetworkPacket *pkt);

    void Send(NetworkPacket* pkt);

    void interact(InteractAction action, const PointedThing &pointed);

    void sendNodemetaFields(v3s16 p, const std::string &formname,
        const StringMap &fields);
    void sendInventoryFields(const std::string &formname,
        const StringMap &fields);
    void sendInventoryAction(InventoryAction *a);
    void sendChatMessage(const std::wstring &message);
    void sendChangePassword(const std::string &oldpassword,
        const std::string &newpassword);
    void sendDamage(u16 damage);
    void sendRespawnLegacy();
    void sendReady();
    void sendHaveMedia(const std::vector<u32> &tokens);
    void sendUpdateClientInfo(const ClientDynamicInfo &info);

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

    // Renaming accessDeniedReason to better name could be good as it's used to
    // disconnect client when CSM failed.
    const std::string &accessDeniedReason() const { return m_access_denied_reason; }

    LocalClientState getState() { return m_state; }

    // IP and port we're connected to
    const Address getServerAddress();

    // Hostname of the connected server (but can also be a numerical IP)
    const std::string &getAddressName() const
    {
        return m_address_name;
    }

    void peerAdded(con::IPeer *peer) override;
    void deletingPeer(con::IPeer *peer, bool timeout) override;
private:
    void initLocalMapSaving(const Address &address,
        const std::string &hostname,
        bool is_local_server);

    void ReceiveAll();

    void sendPlayerPos();

    void sendInit(const std::string &playerName);
    void startAuth(AuthMechanism chosen_auth_mechanism);
    void sendDeletedBlocks(std::vector<v3s16> &blocks);
    void sendGotBlocks(const std::vector<v3s16> &blocks);
    void sendRemovedSounds(const std::vector<s32> &soundList);

    bool canSendChatMessage() const;

};
