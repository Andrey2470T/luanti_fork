#pragma once

#include <BasicIncludes.h>
#include <queue>
#include <memory>
#include "gui/IGUIEnvironment.h"
#include "gui/guiChatConsole.h"
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
    GUIChatConsole *gui_chat_console = nullptr;

	std::queue<std::wstring> out_chat_queue;
	u32 last_chat_message_sent;
	float chat_message_allowance = 5.0f;
	std::queue<ChatMessage *> chat_queue;
public:
#ifdef __ANDROID__
    bool android_chat_open;
#endif

	ChatMessanger(Client *_client);
    ~ChatMessanger();

    ChatBackend *getBackend() const
    {
        return chat_backend.get();
    }
    GUIChatConsole *getChatConsole() const
    {
        return gui_chat_console;
    }

    void init(gui::IGUIEnvironment *guienv);

    void openConsole(f32 scale, const wchar_t *line=NULL);

#ifdef __ANDROID__
    void handleAndroidChatInput();
#endif

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
	
	void updateChat(f32 dtime);

    static void settingChangedCallback(const std::string &setting_name, void *data);
    void readSettings();
};
