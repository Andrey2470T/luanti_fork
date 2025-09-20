#include "chatmessanger.h"
#include "chat.h"
#include "chatmessage.h"
#include "client/network/packethandler.h"
#include "gui/guiChatConsole.h"
#include "client.h"
#include "client/network/packethandler.h"
#include "gui/mainmenumanager.h"
#include "settings.h"
#include "log.h"
#include <ctime>
#include "util/string.h"
#include "script/scripting_client.h"
#include "client/render/rendersystem.h"
#include "client/ui/gameui.h"

ChatMessanger::ChatMessanger(Client *_client)
    : client(_client), packet_handler(client->getPacketHandler()), chat_backend(std::make_unique<ChatBackend>()),
    chat_log_buf(g_logger), last_chat_message_sent(time(nullptr))
{
    g_settings->registerChangedCallback("chat_log_level",
        &settingChangedCallback, this);
}

void ChatMessanger::init(gui::IGUIEnvironment *guienv)
{
    // Remove stale "recent" chat messages from previous connections
    chat_backend->clearRecentChat();

    // Make sure the size of the recent messages buffer is right
    chat_backend->applySettings();

    // Chat backend and console
    gui_chat_console = std::make_unique<GUIChatConsole>(guienv, guienv->getRootGUIElement(),
            -1, chat_backend.get(), client, g_menumgr.get());
}

void ChatMessanger::openConsole(float scale, const wchar_t *line)
{
    assert(scale > 0.0f && scale <= 1.0f);

#ifdef __ANDROID__
    if (!porting::hasPhysicalKeyboardAndroid()) {
        porting::showTextInputDialog("", "", 2);
        android_chat_open = true;
    } else {
#endif
    if (gui_chat_console->isOpenInhibited())
        return;
    gui_chat_console->openConsole(scale);
    if (line) {
        gui_chat_console->setCloseOnEnter(true);
        gui_chat_console->replaceAndAddToHistory(line);
    }
#ifdef __ANDROID__
    } // else
#endif
}

#ifdef __ANDROID__
void ChatMessanger::handleAndroidChatInput()
{
    // It has to be a text input
    if (android_chat_open && porting::getLastInputDialogType() == porting::TEXT_INPUT) {
        porting::AndroidDialogState dialogState = porting::getInputDialogState();
        if (dialogState == porting::DIALOG_INPUTTED) {
            std::string text = porting::getInputDialogMessage();
            typeChatMessage(utf8_to_wide(text));
        }
        if (dialogState != porting::DIALOG_SHOWN)
            android_chat_open = false;
    }
}
#endif

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

void ChatMessanger::updateChat(f32 dtime)
{
	auto color_for = [](LogLevel level) -> const char* {
		switch (level) {
		case LL_ERROR  : return "\x1b(c@#F00)"; // red
		case LL_WARNING: return "\x1b(c@#EE0)"; // yellow
		case LL_INFO   : return "\x1b(c@#BBB)"; // grey
		case LL_VERBOSE: return "\x1b(c@#888)"; // dark grey
		case LL_TRACE  : return "\x1b(c@#888)"; // dark grey
		default        : return "";
		}
	};

	// Get new messages from error log buffer
	std::vector<LogEntry> entries = chat_log_buf.take();
	for (const auto& entry : entries) {
		std::string line;
		line.append(color_for(entry.level)).append(entry.combined);
		chat_backend->addMessage(L"", utf8_to_wide(line));
	}

	// Get new messages from client
	std::wstring message;
	while (getChatMessage(message)) {
		chat_backend->addUnparsedMessage(message);
	}

	// Remove old messages
	chat_backend->step(dtime);

	// Display all messages in a static text element
	auto &buf = chat_backend->getRecentBuffer();
	if (buf.getLinesModified()) {
		buf.resetLinesModified();
        client->getRenderSystem()->getGameUI()->setChatText(chat_backend->getRecentChat(), buf.getLineCount());
	}

	// Make sure that the size is still correct
	client->getRenderSystem()->getGameUI()->updateChatSize();
}

void ChatMessanger::settingChangedCallback(const std::string &setting_name, void *data)
{
    ((ChatMessanger *)data)->readSettings();
}

void ChatMessanger::readSettings()
{
    LogLevel chat_log_level = Logger::stringToLevel(g_settings->get("chat_log_level"));
    if (chat_log_level == LL_MAX) {
        warningstream << "Supplied unrecognized chat_log_level; showing none." << std::endl;
        chat_log_level = LL_NONE;
    }
    chat_log_buf.setLogLevel(chat_log_level);
}
