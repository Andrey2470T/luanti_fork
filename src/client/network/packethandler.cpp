#include "packethandler.h"

#include "gettext.h"
#include "network/connection.h"
#include "network/networkpacket.h"
#include "cmdtable.h"
#include "client/core/client.h"
#include "util/serialize.h"
#include "porting.h"
#include "profiler.h"
#include "client/player/localplayer.h"
#include "client/player/playercamera.h"
#include "inventorymanager.h"
#include "version.h"
#include "clientdynamicinfo.h"
#include "modchannels.h"

/*
    Utility classes
*/

u32 PacketCounter::sum() const
{
    u32 n = 0;
    for (const auto &it : m_packets)
        n += it.second;
    return n;
}

void PacketCounter::print(std::ostream &o) const
{
    for (const auto &it : m_packets) {
        auto name = it.first >= TOCLIENT_NUM_MSG_TYPES ? nullptr
            : toClientCommandTable[it.first].name;
        if (!name)
            name = "?";
        o << "cmd " << it.first << " (" << name << ") count "
            << it.second << std::endl;
    }
}


ClientPacketHandler::~ClientPacketHandler()
{
    if (m_con)
        m_con->Disconnect();
}

void ClientPacketHandler::connect(const Address &address, const std::string &address_name,
    bool is_local_server)
{
    if (m_con) {
        // can't do this if the connection has entered auth phase
        sanity_check(m_client->getState() == LC_Created && m_client->m_proto_ver == 0);
        infostream << "Client connection will be recreated" << std::endl;

        m_access_denied = false;
        m_access_denied_reconnect = false;
        m_access_denied_reason.clear();
    }

    m_address_name = address_name;
    m_con.reset(con::createMTP(CONNECTION_TIMEOUT, address.isIPv6(), this));

    infostream << "Connecting to server at ";
    address.print(infostream);
    infostream << std::endl;

    m_con->Connect(address);
}

inline void ClientPacketHandler::handleCommand(NetworkPacket* pkt)
{
    const ToClientCommandHandler& opHandle = toClientCommandTable[pkt->getCommand()];
    (this->*opHandle.handler)(pkt);
}

/*
    sender_peer_id given to this shall be quaranteed to be a valid peer
*/
void ClientPacketHandler::processData(NetworkPacket *pkt)
{
    ToClientCommand command = (ToClientCommand) pkt->getCommand();

    m_packetcounter.add(static_cast<u16>(command));
    g_profiler->graphAdd("client_received_packets", 1);

    /*
        If this check is removed, be sure to change the queue
        system to know the ids
    */
    if (pkt->getPeerId() != PEER_ID_SERVER) {
        infostream << "Client::ProcessData(): Discarding data not "
            "coming from server: peer_id=" << static_cast<int>(pkt->getPeerId())
            << " command=" << static_cast<unsigned>(command) << std::endl;
        return;
    }

    // Command must be handled into ToClientCommandHandler
    if (command >= TOCLIENT_NUM_MSG_TYPES) {
        infostream << "Client: Ignoring unknown command "
            << static_cast<unsigned>(command) << std::endl;
        return;
    }

    /*
     * Those packets are handled before m_server_ser_ver is set, it's normal
     * But we must use the new ToClientConnectionState in the future,
     * as a byte mask
     */
    if (toClientCommandTable[command].state == TOCLIENT_STATE_NOT_CONNECTED) {
        handleCommand(pkt);
        return;
    }

    if (m_server_ser_ver == SER_FMT_VER_INVALID) {
        infostream << "Client: Server serialization"
                " format invalid. Skipping incoming command "
                << static_cast<unsigned>(command) << std::endl;
        return;
    }

    handleCommand(pkt);
}

void ClientPacketHandler::send(NetworkPacket* pkt)
{
    auto &scf = serverCommandFactoryTable[pkt->getCommand()];
    FATAL_ERROR_IF(!scf.name, "packet type missing in table");
    m_con->Send(PEER_ID_SERVER, scf.channel, pkt, scf.reliable);
}

// Will fill up 12 + 12 + 4 + 4 + 4 + 1 + 1 + 1 + 4 + 4 bytes
void writePlayerPos(LocalPlayer *myplayer, f32 range, NetworkPacket *pkt)
{
    v3f pf           = myplayer->getPosition() * 100;
    v3f sf           = myplayer->getSpeed() * 100;
    s32 pitch        = myplayer->getPitch() * 100;
    s32 yaw          = myplayer->getYaw() * 100;
    u32 keyPressed   = myplayer->control.getKeysPressed();
    // scaled by 80, so that pi can fit into a u8
    u8 fov           = std::fmin(255.0f, myplayer->getCamera()->getFovMax() * 80.0f);
    u8 wanted_range  = std::fmin(255.0f,
            std::ceil(range * (1.0f / MAP_BLOCKSIZE)));
    f32 movement_speed = myplayer->control.movement_speed;
    f32 movement_dir = myplayer->control.movement_direction;

    v3i position(pf.X, pf.Y, pf.Z);
    v3i speed(sf.X, sf.Y, sf.Z);

    /*
        Format:
        [0] v3s32 position*100
        [12] v3s32 speed*100
        [12+12] s32 pitch*100
        [12+12+4] s32 yaw*100
        [12+12+4+4] u32 keyPressed
        [12+12+4+4+4] u8 fov*80
        [12+12+4+4+4+1] u8 ceil(wanted_range / MAP_BLOCKSIZE)
        [12+12+4+4+4+1+1] u8 camera_inverted (bool)
        [12+12+4+4+4+1+1+1] f32 movement_speed
        [12+12+4+4+4+1+1+1+4] f32 movement_direction
    */
    *pkt << position << speed << pitch << yaw << keyPressed;
    *pkt << fov << wanted_range;
    *pkt << (myplayer->getCamera()->getCameraMode() == CAMERA_MODE_THIRD_FRONT);
    *pkt << movement_speed << movement_dir;
}

void ClientPacketHandler::interact(f32 range, InteractAction action, const PointedThing& pointed)
{
    if(m_client->getState() != LC_Ready) {
        errorstream << "Client::interact() "
                "Canceled (not connected)"
                << std::endl;
        return;
    }

    LocalPlayer *myplayer = m_client->getEnv().getLocalPlayer();
    if (!myplayer)
        return;

    /*
        [0] u16 command
        [2] u8 action
        [3] u16 item
        [5] u32 length of the next item (plen)
        [9] serialized PointedThing
        [9 + plen] player position information
    */

    NetworkPacket pkt(TOSERVER_INTERACT, 1 + 2 + 0);

    pkt << (u8)action;
    pkt << myplayer->getWieldIndex();

    std::ostringstream tmp_os(std::ios::binary);
    pointed.serialize(tmp_os);

    pkt.putLongString(tmp_os.str());

    writePlayerPos(myplayer, range, &pkt);

    send(&pkt);
}

void ClientPacketHandler::sendNodemetaFields(v3s16 p, const std::string &formname,
        const StringMap &fields)
{
    size_t fields_size = fields.size();

    FATAL_ERROR_IF(fields_size > 0xFFFF, "Unsupported number of nodemeta fields");

    NetworkPacket pkt(TOSERVER_NODEMETA_FIELDS, 0);

    pkt << p << formname << (u16) (fields_size & 0xFFFF);

    StringMap::const_iterator it;
    for (it = fields.begin(); it != fields.end(); ++it) {
        const std::string &name = it->first;
        const std::string &value = it->second;
        pkt << name;
        pkt.putLongString(value);
    }

    send(&pkt);
}

void ClientPacketHandler::sendInventoryFields(const std::string &formname,
        const StringMap &fields)
{
    size_t fields_size = fields.size();
    FATAL_ERROR_IF(fields_size > 0xFFFF, "Unsupported number of inventory fields");

    NetworkPacket pkt(TOSERVER_INVENTORY_FIELDS, 0);
    pkt << formname << (u16) (fields_size & 0xFFFF);

    StringMap::const_iterator it;
    for (it = fields.begin(); it != fields.end(); ++it) {
        const std::string &name  = it->first;
        const std::string &value = it->second;
        pkt << name;
        pkt.putLongString(value);
    }

    send(&pkt);
}

void ClientPacketHandler::sendInventoryAction(InventoryAction *a)
{
    std::ostringstream os(std::ios_base::binary);

    a->serialize(os);

    // Make data buffer
    std::string s = os.str();

    NetworkPacket pkt(TOSERVER_INVENTORY_ACTION, s.size());
    pkt.putRawString(s.c_str(),s.size());

    send(&pkt);

    a->clientApply(m_client, m_client);

    // Remove it
    delete a;
}

void ClientPacketHandler::sendChatMessage(const std::wstring &message)
{
    NetworkPacket pkt(TOSERVER_CHAT_MESSAGE, 2 + message.size() * sizeof(u16));
    pkt << message;
    send(&pkt);
}

void ClientPacketHandler::sendDamage(u16 damage)
{
    NetworkPacket pkt(TOSERVER_DAMAGE, sizeof(u16));
    pkt << damage;
    send(&pkt);
}

void ClientPacketHandler::sendRespawnLegacy()
{
    NetworkPacket pkt(TOSERVER_RESPAWN_LEGACY, 0);
    send(&pkt);
}

void ClientPacketHandler::sendReady()
{
    NetworkPacket pkt(TOSERVER_CLIENT_READY,
            1 + 1 + 1 + 1 + 2 + sizeof(char) * strlen(g_version_hash) + 2);

    pkt << (u8) VERSION_MAJOR << (u8) VERSION_MINOR << (u8) VERSION_PATCH
        << (u8) 0 << (u16) strlen(g_version_hash);

    pkt.putRawString(g_version_hash, (u16) strlen(g_version_hash));
    pkt << (u16)FORMSPEC_API_VERSION;
    send(&pkt);
}

void ClientPacketHandler::sendHaveMedia(const std::vector<u32> &tokens)
{
    NetworkPacket pkt(TOSERVER_HAVE_MEDIA, 1 + tokens.size() * 4);

    sanity_check(tokens.size() < 256);

    pkt << static_cast<u8>(tokens.size());
    for (u32 token : tokens)
        pkt << token;

    send(&pkt);
}

void ClientPacketHandler::sendUpdateClientInfo(const ClientDynamicInfo& info)
{
    NetworkPacket pkt(TOSERVER_UPDATE_CLIENT_INFO, 4*2 + 4 + 4 + 4*2);
    pkt << (u32)info.render_target_size.X << (u32)info.render_target_size.Y;
    pkt << info.real_gui_scaling;
    pkt << info.real_hud_scaling;
    pkt << (f32)info.max_fs_size.X << (f32)info.max_fs_size.Y;
    pkt << info.touch_controls;

    send(&pkt);
}

void ClientPacketHandler::sendPlayerItem(u16 item)
{
    auto player = m_client->getEnv().getLocalPlayer();
    player->setWieldIndex(item);
    player->updateWieldedItem();

    NetworkPacket pkt(TOSERVER_PLAYERITEM, 2);
    pkt << item;
    send(&pkt);
}

bool ClientPacketHandler::checkConnection(std::string *error_msg, bool *reconnect_requested)
{
    if (accessDenied()) {
        *error_msg = fmtgettext("Access denied. Reason: %s", accessDeniedReason().c_str());
        *reconnect_requested = reconnectRequested();
        errorstream << *error_msg << std::endl;
        return false;
    }

    return true;
}

const Address ClientPacketHandler::getServerAddress()
{
    return m_con ? m_con->GetPeerAddress(PEER_ID_SERVER) : Address();
}

float ClientPacketHandler::getRTT()
{
    assert(m_con);
    return m_con->getPeerStat(PEER_ID_SERVER,con::AVG_RTT);
}

float ClientPacketHandler::getCurRate()
{
    assert(m_con);
    return (m_con->getLocalStat(con::CUR_INC_RATE) +
            m_con->getLocalStat(con::CUR_DL_RATE));
}

bool ClientPacketHandler::hasServerReplied() const {
    return m_client->m_proto_ver != 0; // (set in TOCLIENT_HELLO)
}

void ClientPacketHandler::printPacketCounter(f32 dtime)
{
    float &counter = m_packetcounter_timer;
    counter -= dtime;
    if(counter <= 0.0f)
    {
        counter = 30.0f;
        u32 sum = m_packetcounter.sum();
        float avg = sum / counter;

        infostream << "Client packetcounter (" << counter << "s): "
                << "sum=" << sum << " avg=" << avg << "/s" << std::endl;
        m_packetcounter.print(infostream);
        m_packetcounter.clear();
    }
}

// Virtual methods from con::PeerHandler
void ClientPacketHandler::peerAdded(con::IPeer *peer)
{
    infostream << "ClientPacketHandler::peerAdded(): peer->id="
            << peer->id << std::endl;
}

void ClientPacketHandler::deletingPeer(con::IPeer *peer, bool timeout)
{
    infostream << "ClientPacketHandler::deletingPeer(): "
            "Server Peer is getting deleted "
            << "(timeout=" << timeout << ")" << std::endl;

    m_access_denied = true;
    if (timeout)
        m_access_denied_reason = gettext("Connection timed out.");
    else if (m_access_denied_reason.empty())
        m_access_denied_reason = gettext("Connection aborted (protocol error?).");
}

void ClientPacketHandler::request_media(const std::vector<std::string> &file_requests)
{
    std::ostringstream os(std::ios_base::binary);
    writeU16(os, TOSERVER_REQUEST_MEDIA);
    size_t file_requests_size = file_requests.size();

    FATAL_ERROR_IF(file_requests_size > 0xFFFF, "Unsupported number of file requests");

    // Packet dynamicly resized
    NetworkPacket pkt(TOSERVER_REQUEST_MEDIA, 2 + 0);

    pkt << (u16) (file_requests_size & 0xFFFF);

    for (const std::string &file_request : file_requests) {
        pkt << file_request;
    }

    send(&pkt);

    infostream << "Client: Sending media request list to server ("
            << file_requests.size() << " files, packet size "
            << pkt.getSize() << ")" << std::endl;
}

void ClientPacketHandler::receiveAll()
{
    NetworkPacket pkt;
    u64 start_ms = porting::getTimeMs();
    const u64 budget = 10;

    FATAL_ERROR_IF(!m_con, "Networking not initialized");
    for(;;) {
        // Limit time even if there would be huge amounts of data to
        // process
        if (porting::getTimeMs() > start_ms + budget) {
            infostream << "Client::ReceiveAll(): "
                    "Packet processing budget exceeded." << std::endl;
            break;
        }

        pkt.clear();
        try {
            if (!m_con->TryReceive(&pkt))
                break;
            processData(&pkt);
        } catch (const con::InvalidIncomingDataException &e) {
            infostream << "Client::ReceiveAll(): "
                    "InvalidIncomingDataException: what()="
                     << e.what() << std::endl;
        }
    }
}

void ClientPacketHandler::sendPlayerPos(f32 range, f32 dtime)
{
    float &counter = m_playerpos_send_timer;
    counter += dtime;
    if(!((m_client->getState() == LC_Ready) && (counter >= m_recommended_send_interval)))
        return;

    counter = 0.0;
    LocalPlayer *player = m_client->getEnv().getLocalPlayer();
    if (!player)
        return;

    // Save bandwidth by only updating position when
    // player is not dead and something changed

    if (m_activeobjects_received && player->isDead())
        return;

    auto camera = player->getCamera();
    u8 camera_fov   = std::fmin(255.0f, camera->getFovMax() * 80.0f);
    u8 wanted_range = std::fmin(255.0f,
            std::ceil(range * (1.0f / MAP_BLOCKSIZE)));

    u32 keyPressed = player->control.getKeysPressed();
    bool camera_inverted = camera->getCameraMode() == CAMERA_MODE_THIRD_FRONT;
    f32 movement_speed = player->control.movement_speed;
    f32 movement_dir = player->control.movement_direction;

    if (
            player->last_position        == player->getPosition() &&
            player->last_speed           == player->getSpeed()    &&
            player->last_pitch           == player->getPitch()    &&
            player->last_yaw             == player->getYaw()      &&
            player->last_keyPressed      == keyPressed            &&
            player->last_camera_fov      == camera_fov            &&
            player->last_camera_inverted == camera_inverted       &&
            player->last_wanted_range    == wanted_range          &&
            player->last_movement_speed  == movement_speed        &&
            player->last_movement_dir    == movement_dir)
        return;

    player->last_position        = player->getPosition();
    player->last_speed           = player->getSpeed();
    player->last_pitch           = player->getPitch();
    player->last_yaw             = player->getYaw();
    player->last_keyPressed      = keyPressed;
    player->last_camera_fov      = camera_fov;
    player->last_camera_inverted = camera_inverted;
    player->last_wanted_range    = wanted_range;
    player->last_movement_speed  = movement_speed;
    player->last_movement_dir    = movement_dir;

    NetworkPacket pkt(TOSERVER_PLAYERPOS, 12 + 12 + 4 + 4 + 4 + 1 + 1 + 1 + 4 + 4);

    writePlayerPos(player, range, &pkt);

    send(&pkt);
}

void ClientPacketHandler::sendInit(f32 dtime)
{
    float &counter = m_connection_reinit_timer;
    counter -= dtime;
    if (counter <= 0) {
        counter = 1.5f;

        LocalPlayer *myplayer = m_client->getEnv().getLocalPlayer();
        FATAL_ERROR_IF(!myplayer, "Local player not found in environment");

        auto playerName = myplayer->getName();
        NetworkPacket pkt(TOSERVER_INIT, 1 + 2 + 2 + (1 + playerName.size()));

        pkt << SER_FMT_VER_HIGHEST_READ << (u16) 0 /* unused */;
        pkt << CLIENT_PROTOCOL_VERSION_MIN << LATEST_PROTOCOL_VERSION;
        pkt << playerName;

        send(&pkt);
    }
}

void ClientPacketHandler::sendAuthFirstSRP(std::string verifier, std::string salt, bool pwd_empty)
{
    NetworkPacket resp_pkt(TOSERVER_FIRST_SRP, 0);
    resp_pkt << salt << verifier << (u8)(pwd_empty ? 1 : 0);

    send(&resp_pkt);
}

void ClientPacketHandler::sendAuthSRPBytesA(char *bytes_A, size_t len_A, u8 based_on)
{
    NetworkPacket resp_pkt(TOSERVER_SRP_BYTES_A, 0);
    resp_pkt << std::string(bytes_A, len_A) << based_on;
    send(&resp_pkt);
}

void ClientPacketHandler::sendDeletedBlocks(std::vector<v3s16> &blocks)
{
    NetworkPacket pkt(TOSERVER_DELETEDBLOCKS, 1 + sizeof(v3s16) * blocks.size());

    pkt << (u8) blocks.size();

    for (const v3s16 &block : blocks) {
        pkt << block;
    }

    send(&pkt);
}

void ClientPacketHandler::sendGotBlocks(const std::vector<v3s16> &blocks)
{
    NetworkPacket pkt(TOSERVER_GOTBLOCKS, 1 + 6 * blocks.size());
    pkt << (u8) blocks.size();
    for (const v3s16 &block : blocks)
        pkt << block;

    send(&pkt);
}

void ClientPacketHandler::sendRemovedSounds(const std::vector<s32> &soundList)
{
    size_t server_ids = soundList.size();
    assert(server_ids <= 0xFFFF);

    NetworkPacket pkt(TOSERVER_REMOVED_SOUNDS, 2 + server_ids * 4);

    pkt << (u16) (server_ids & 0xFFFF);

    for (s32 sound_id : soundList)
        pkt << sound_id;

    send(&pkt);
}

bool ClientPacketHandler::joinModChannel(ModChannelMgr *mochannel, const std::string &channel)
{
    if (mochannel->channelRegistered(channel))
        return false;

    NetworkPacket pkt(TOSERVER_MODCHANNEL_JOIN, 2 + channel.size());
    pkt << channel;
    send(&pkt);

    mochannel->joinChannel(channel, 0);
    return true;
}

bool ClientPacketHandler::leaveModChannel(ModChannelMgr *mochannel, const std::string &channel)
{
    if (!mochannel->channelRegistered(channel))
        return false;

    NetworkPacket pkt(TOSERVER_MODCHANNEL_LEAVE, 2 + channel.size());
    pkt << channel;
    send(&pkt);

    mochannel->leaveChannel(channel, 0);
    return true;
}

bool ClientPacketHandler::sendModChannelMessage(ModChannelMgr *mochannel, const std::string &channel, const std::string &message)
{
    if (!mochannel->canWriteOnChannel(channel))
        return false;

    if (message.size() > STRING_MAX_LEN) {
        warningstream << "ModChannel message too long, dropping before sending "
                << " (" << message.size() << " > " << STRING_MAX_LEN << ", channel: "
                << channel << ")" << std::endl;
        return false;
    }

    // @TODO: do some client rate limiting
    NetworkPacket pkt(TOSERVER_MODCHANNEL_MSG, 2 + channel.size() + 2 + message.size());
    pkt << channel << message;
    send(&pkt);
    return true;
}
