#pragma once

#include <BasicIncludes.h>
#include <queue>
#include <memory>
#include "gui/IGUIEnvironment.h"
#include "log_internal.h"

class ClientPacketHandler;
class ChatBackend;
class GUIChatConsole;
struct ChatMessage;
class Client;

namespace gui
{
class IGUIEnvironment;
}

class ChatMessanger
{
	Client *client;
	ClientPacketHandler *packet_handler;

	std::unique_ptr<ChatBackend> chat_backend;
    CaptureLogOutput chat_log_buf;
    std::unique_ptr<GUIChatConsole> gui_chat_console;

	std::queue<std::wstring> out_chat_queue;
	u32 last_chat_message_sent;
	float chat_message_allowance = 5.0f;
	std::queue<ChatMessage *> chat_queue;
public:
	ChatMessanger(Client *_client);

    void init(gui::IGUIEnvironment *guienv);

	bool getChatMessage(std::wstring &message);
    void typeChatMessage(const std::wstring& message);

	void pushToChatQueue(ChatMessage *cec)
	{
		chat_queue.push(cec);
	}

	bool canSendChatMessage() const;
    void sendChatMessage(const std::wstring &message);
    void clearOutChatQueue();

	void sendFromQueue();
};
