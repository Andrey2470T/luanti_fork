#include "chatmessanger.h"
#include "chat.h"
#include "chatmessage.h"
#include "client/network/packethandler.h"
#include "gui/guiChatConsole.h"
#include "client.h"
#include "clientpackethandler.h"
#include "gui/mainmenumanager.h"
#include "settings.h"
#include "log.h"
#include <ctime.h>
#include "utils/string.h"
#include "script/scripting_client.h"

ChatMessanger::ChatMessanger(Client *_client)
    : client(_client), packet_handler(client->getPacketHandler()), chat_backend(std::make_unique<ChatBackend>()),
    last_chat_message_sent(time(nullptr))
{}

void ChatMessanger::init(gui::IGUIEnvironment *guienv)
{
    // Remove stale "recent" chat messages from previous connections
    chat_backend->clearRecentChat();

    // Make sure the size of the recent messages buffer is right
    chat_backend->applySettings();

    // Chat backend and console
    gui_chat_console = std::make_unique<GUIChatConsole>(guienv, guienv->getRootGUIElement(),
            -1, chat_backend, client, &g_menumgr);
}

bool ChatMessanger::getChatMessage(std::wstring &res)
{
	if (chat_queue.empty())
		return false;

	ChatMessage *chatMessage = chat_queue.front();
	chat_queue.pop();

	res = L"";

	switch (chatMessage->type) {
		case CHATMESSAGE_TYPE_RAW:
		case CHATMESSAGE_TYPE_ANNOUNCE:
		case CHATMESSAGE_TYPE_SYSTEM:
			res = chatMessage->message;
			break;
		case CHATMESSAGE_TYPE_NORMAL: {
			if (!chatMessage->sender.empty())
				res = L"<" + chatMessage->sender + L"> " + chatMessage->message;
			else
				res = chatMessage->message;
			break;
		}
		default:
			break;
	}

	delete chatMessage;
	return true;
}

void ChatMessanger::typeChatMessage(const std::wstring &message)
{
	// Discard empty line
	if (message.empty())
		return;

	auto message_utf8 = wide_to_utf8(message);
	infostream << "Typed chat message: \"" << message_utf8 << "\"" << std::endl;

	// If message was consumed by script API, don't send it to server
	if (client->modsLoaded() && client->getScript()->on_sending_message(message_utf8))
		return;

	// Send to others
	sendChatMessage(message);
}


bool ChatMessanger::canSendChatMessage() const
{
	u32 now = time(nullptr);
	float time_passed = now - last_chat_message_sent;

	float virt_chat_message_allowance = chat_message_allowance + time_passed *
			(CLIENT_CHAT_MESSAGE_LIMIT_PER_10S / 8.0f);

	if (virt_chat_message_allowance < 1.0f)
		return false;

	return true;
}

void ChatMessanger::sendChatMessage(const std::wstring &message)
{
	const s16 max_queue_size = g_settings->getS16("max_out_chat_queue_size");
	if (canSendChatMessage()) {
		u32 now = time(NULL);
		float time_passed = now - last_chat_message_sent;
		last_chat_message_sent = now;

		chat_message_allowance += time_passed * (CLIENT_CHAT_MESSAGE_LIMIT_PER_10S / 8.0f);
		if (chat_message_allowance > CLIENT_CHAT_MESSAGE_LIMIT_PER_10S)
			chat_message_allowance = CLIENT_CHAT_MESSAGE_LIMIT_PER_10S;

		chat_message_allowance -= 1.0f;

        packet_handler->sendChatMessage(message);
	} else if (out_chat_queue.size() < (u16) max_queue_size || max_queue_size < 0) {
		out_chat_queue.push(message);
	} else {
		infostream << "Could not queue chat message because maximum out chat queue size ("
				<< max_queue_size << ") is reached." << std::endl;
	}
}

void ChatMessanger::clearOutChatQueue()
{
	out_chat_queue = std::queue<std::wstring>();
}

void ChatMessanger::sendFromQueue()
{
	if (out_chat_queue.empty() || !canSendChatMessage())
		return;

	sendChatMessage(out_chat_queue.front());
	out_chat_queue.pop();
}
