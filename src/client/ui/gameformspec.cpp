// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#include "gameformspec.h"

#include "gettext.h"
#include "nodemetadata.h"
#include "client/render/rendersystem.h"
#include "client/event/inputhandler.h"
#include "client/core/client.h"
#include "client/player/localplayer.h"
#include "scripting_client.h"
#include "client/map/clientmap.h"
#include "gui/guiFormSpecMenu.h"
#include "gui/mainmenumanager.h"
#include "gui/touchcontrols.h"
#include "gui/touchscreeneditor.h"
#include "gui/guiPasswordChange.h"
#include "gui/guiKeyChangeMenu.h"
#include "gui/guiPasswordChange.h"
#include "gui/guiOpenURL.h"
#include "gui/guiVolumeChange.h"
#include "settings.h"
#include "client/map/clientmap.h"
#include "client/network/packethandler.h"

/*
	Text input system
*/

struct TextDestNodeMetadata : public TextDest
{
	TextDestNodeMetadata(v3s16 p, Client *client)
	{
		m_p = p;
		m_client = client;
	}
	// This is deprecated I guess? -celeron55
	void gotText(const std::wstring &text)
	{
		std::string ntext = wide_to_utf8(text);
		infostream << "Submitting 'text' field of node at (" << m_p.X << ","
			   << m_p.Y << "," << m_p.Z << "): " << ntext << std::endl;
		StringMap fields;
		fields["text"] = ntext;
        m_client->getPacketHandler()->sendNodemetaFields(m_p, "", fields);
	}
	void gotText(const StringMap &fields)
	{
        m_client->getPacketHandler()->sendNodemetaFields(m_p, "", fields);
	}

	v3s16 m_p;
	Client *m_client;
};

struct TextDestPlayerInventory : public TextDest
{
	TextDestPlayerInventory(Client *client)
	{
		m_client = client;
		m_formname.clear();
	}
	TextDestPlayerInventory(Client *client, const std::string &formname)
	{
		m_client = client;
		m_formname = formname;
	}
	void gotText(const StringMap &fields)
	{
        m_client->getPacketHandler()->sendInventoryFields(m_formname, fields);
	}

	Client *m_client;
};

struct LocalFormspecHandler : public TextDest
{
	LocalFormspecHandler(const std::string &formname)
	{
		m_formname = formname;
	}

	LocalFormspecHandler(const std::string &formname, Client *client):
		m_client(client)
	{
		m_formname = formname;
	}

	void gotText(const StringMap &fields)
	{
		if (m_formname == "MT_PAUSE_MENU") {
			if (fields.find("btn_sound") != fields.end()) {
				g_gamecallback->changeVolume();
				return;
			}

			if (fields.find("btn_key_config") != fields.end()) {
				g_gamecallback->keyConfig();
				return;
			}

			if (fields.find("btn_touchscreen_layout") != fields.end()) {
				g_gamecallback->touchscreenLayout();
				return;
			}

			if (fields.find("btn_exit_menu") != fields.end()) {
				g_gamecallback->disconnect();
				return;
			}

			if (fields.find("btn_exit_os") != fields.end()) {
				g_gamecallback->exitToOS();
#ifndef __ANDROID__
                m_client->getRenderSystem()->getWindow()->close();
#endif
				return;
			}

			if (fields.find("btn_change_password") != fields.end()) {
				g_gamecallback->changePassword();
				return;
			}

			return;
		}

		if (m_formname == "MT_DEATH_SCREEN") {
			assert(m_client != nullptr);

			if (fields.find("quit") != fields.end())
                m_client->getPacketHandler()->sendRespawnLegacy();

			return;
		}

		if (m_client->modsLoaded())
			m_client->getScript()->on_formspec_input(m_formname, fields);
	}

	Client *m_client = nullptr;
};

/* Form update callback */

class NodeMetadataFormSource: public IFormSource
{
public:
	NodeMetadataFormSource(ClientMap *map, v3s16 p):
		m_map(map),
		m_p(p)
	{
	}
	const std::string &getForm() const
	{
		static const std::string empty_string = "";
		NodeMetadata *meta = m_map->getNodeMetadata(m_p);

		if (!meta)
			return empty_string;

		return meta->getString("formspec");
	}

	virtual std::string resolveText(const std::string &str)
	{
		NodeMetadata *meta = m_map->getNodeMetadata(m_p);

		if (!meta)
			return str;

		return meta->resolveString(str);
	}

	ClientMap *m_map;
	v3s16 m_p;
};

class PlayerInventoryFormSource: public IFormSource
{
public:
	PlayerInventoryFormSource(Client *client):
		m_client(client)
	{
	}

	const std::string &getForm() const
	{
		LocalPlayer *player = m_client->getEnv().getLocalPlayer();
		return player->inventory_formspec;
	}

	Client *m_client;
};


//// GameFormSpec

void GameFormSpec::deleteFormspec()
{
    if (formspec) {
        formspec->drop();
        formspec = nullptr;
	}
    formname.clear();
}

GameFormSpec::~GameFormSpec() {
    if (formspec)
        formspec->quitMenu();
	this->deleteFormspec();
}

void GameFormSpec::showFormSpec(const std::string &_formspec, const std::string &_formname)
{
    if (_formspec.empty()) {
        if (formname.empty() || formname == formname) {
            formspec->quitMenu();
		}
	} else {
		FormspecFormSource *fs_src =
			new FormspecFormSource(formspec);
		TextDestPlayerInventory *txt_dst =
            new TextDestPlayerInventory(client, formname);

        formname = _formname;
        GUIFormSpecMenu::create(formspec, client, rndsys->getGUIEnvironment(),
            &input->joystick, fs_src, txt_dst, client->getFormspecPrepend(),
            client->getSoundManager());
	}
}

void GameFormSpec::showLocalFormSpec(const std::string &_formspec, const std::string &_formname)
{
	FormspecFormSource *fs_src = new FormspecFormSource(formspec);
	LocalFormspecHandler *txt_dst =
        new LocalFormspecHandler(formname, client);
    GUIFormSpecMenu::create(formspec, client, rndsys->getGUIEnvironment(),
            &input->joystick, fs_src, txt_dst, client->getFormspecPrepend(),
            client->getSoundManager());
}

void GameFormSpec::showNodeFormspec(const std::string &_formspec, const v3s16 &nodepos)
{
	infostream << "Launching custom inventory view" << std::endl;

	InventoryLocation inventoryloc;
	inventoryloc.setNodeMeta(nodepos);

	NodeMetadataFormSource *fs_src = new NodeMetadataFormSource(
        &client->getEnv().getClientMap(), nodepos);
    TextDest *txt_dst = new TextDestNodeMetadata(nodepos, client);

    formname = "";
    GUIFormSpecMenu::create(formspec, client, rndsys->getGUIEnvironment(),
        &input->joystick, fs_src, txt_dst, client->getFormspecPrepend(),
        client->getSoundManager());

    formspec->setFormSpec(_formspec, inventoryloc);
}

void GameFormSpec::showPlayerInventory()
{
	/*
	 * Don't permit to open inventory is CAO or player doesn't exists.
	 * This prevent showing an empty inventory at player load
	 */

    LocalPlayer *player = client->getEnv().getLocalPlayer();
	if (!player || !player->getCAO())
		return;

	infostream << "Game: Launching inventory" << std::endl;

    PlayerInventoryFormSource *fs_src = new PlayerInventoryFormSource(client);

	InventoryLocation inventoryloc;
	inventoryloc.setCurrentPlayer();

    if (client->modsLoaded() && client->getScript()->on_inventory_open(client->getInventory(inventoryloc))) {
		delete fs_src;
		return;
	}

	if (fs_src->getForm().empty()) {
		delete fs_src;
		return;
	}

    TextDest *txt_dst = new TextDestPlayerInventory(client);
    formname = "";
    GUIFormSpecMenu::create(formspec, client, rndsys->getGUIEnvironment(),
        &input->joystick, fs_src, txt_dst, client->getFormspecPrepend(),
        client->getSoundManager());

    formspec->setFormSpec(fs_src->getForm(), inventoryloc);
}

#define SIZE_TAG "size[11,5.5,true]" // Fixed size (ignored in touchscreen mode)

void GameFormSpec::showPauseMenu()
{
	std::string control_text;

	if (g_touchcontrols) {
		control_text = strgettext("Controls:\n"
			"No menu open:\n"
			"- slide finger: look around\n"
			"- tap: place/punch/use (default)\n"
			"- long tap: dig/use (default)\n"
			"Menu/inventory open:\n"
			"- double tap (outside):\n"
			" --> close\n"
			"- touch stack, touch slot:\n"
			" --> move stack\n"
			"- touch&drag, tap 2nd finger\n"
			" --> place single item to slot\n"
			);
	}

    auto simple_singleplayer_mode = client->m_simple_singleplayer_mode;

	float ypos = simple_singleplayer_mode ? 0.7f : 0.1f;
	std::ostringstream os;

	os << "formspec_version[1]" << SIZE_TAG
		<< "button_exit[4," << (ypos++) << ";3,0.5;btn_continue;"
		<< strgettext("Continue") << "]";

	if (!simple_singleplayer_mode) {
		os << "button_exit[4," << (ypos++) << ";3,0.5;btn_change_password;"
			<< strgettext("Change Password") << "]";
	} else {
		os << "field[4.95,0;5,1.5;;" << strgettext("Game paused") << ";]";
	}

#ifndef __ANDROID__
#if USE_SOUND
	os << "button_exit[4," << (ypos++) << ";3,0.5;btn_sound;"
		<< strgettext("Sound Volume") << "]";
#endif
#endif

	if (g_touchcontrols) {
		os << "button_exit[4," << (ypos++) << ";3,0.5;btn_touchscreen_layout;"
			<< strgettext("Touchscreen Layout")  << "]";
	} else {
		os << "button_exit[4," << (ypos++) << ";3,0.5;btn_key_config;"
			<< strgettext("Controls")  << "]";
	}
	os		<< "button_exit[4," << (ypos++) << ";3,0.5;btn_exit_menu;"
		<< strgettext("Exit to Menu") << "]";
	os		<< "button_exit[4," << (ypos++) << ";3,0.5;btn_exit_os;"
		<< strgettext("Exit to OS")   << "]";
	if (!control_text.empty()) {
	os		<< "textarea[7.5,0.25;3.9,6.25;;" << control_text << ";]";
	}
	os		<< "textarea[0.4,0.25;3.9,6.25;;" << PROJECT_NAME_C " " VERSION_STRING "\n"
		<< "\n"
		<<  strgettext("Game info:") << "\n";
    const std::string &address = client->getPacketHandler()->getAddressName();
	os << strgettext("- Mode: ");
	if (!simple_singleplayer_mode) {
		if (address.empty())
			os << strgettext("Hosting server");
		else
			os << strgettext("Remote server");
	} else {
		os << strgettext("Singleplayer");
	}
	os << "\n";
	if (simple_singleplayer_mode || address.empty()) {
		static const std::string on = strgettext("On");
		static const std::string off = strgettext("Off");
		// Note: Status of enable_damage and creative_mode settings is intentionally
		// NOT shown here because the game might roll its own damage system and/or do
		// a per-player Creative Mode, in which case writing it here would mislead.
		bool damage = g_settings->getBool("enable_damage");
		const std::string &announced = g_settings->getBool("server_announce") ? on : off;
		if (!simple_singleplayer_mode) {
			if (damage) {
				const std::string &pvp = g_settings->getBool("enable_pvp") ? on : off;
				//~ PvP = Player versus Player
				os << strgettext("- PvP: ") << pvp << "\n";
			}
			os << strgettext("- Public: ") << announced << "\n";
			std::string server_name = g_settings->get("server_name");
			str_formspec_escape(server_name);
			if (announced == on && !server_name.empty())
				os << strgettext("- Server Name: ") << server_name;

		}
	}
	os << ";]";

	/* Create menu */
	/* Note: FormspecFormSource and LocalFormspecHandler  *
	 * are deleted by guiFormSpecMenu                     */
	FormspecFormSource *fs_src = new FormspecFormSource(os.str());
	LocalFormspecHandler *txt_dst = new LocalFormspecHandler("MT_PAUSE_MENU");

    GUIFormSpecMenu::create(formspec, client, rndsys->getGUIEnvironment(),
            &input->joystick, fs_src, txt_dst, client->getFormspecPrepend(),
            client->getSoundManager());
    formspec->setFocus("btn_continue");
	// game will be paused in next step, if in singleplayer (see m_is_paused)
    formspec->doPause = true;
}

void GameFormSpec::showDeathFormspecLegacy()
{
	static std::string formspec_str =
		std::string("formspec_version[1]") +
		SIZE_TAG
		"bgcolor[#320000b4;true]"
		"label[4.85,1.35;" + gettext("You died") + "]"
		"button_exit[4,3;3,0.5;btn_respawn;" + gettext("Respawn") + "]"
		;

	/* Create menu */
	/* Note: FormspecFormSource and LocalFormspecHandler  *
	 * are deleted by guiFormSpecMenu                     */
	FormspecFormSource *fs_src = new FormspecFormSource(formspec_str);
    LocalFormspecHandler *txt_dst = new LocalFormspecHandler("MT_DEATH_SCREEN", client);

    GUIFormSpecMenu::create(formspec, client, rndsys->getGUIEnvironment(),
        &input->joystick, fs_src, txt_dst, client->getFormspecPrepend(),
        client->getSoundManager());
    formspec->setFocus("btn_respawn");
}

void GameFormSpec::update()
{
	/*
	   make sure menu is on top
	   1. Delete formspec menu reference if menu was removed
	   2. Else, make sure formspec menu is on top
	*/
    if (!formspec)
		return;

    if (formspec->getReferenceCount() == 1) {
		// See GUIFormSpecMenu::create what refcnt = 1 means
		this->deleteFormspec();
		return;
	}

    auto &loc = formspec->getFormspecLocation();
	if (loc.type == InventoryLocation::NODEMETA) {
        NodeMetadata *meta = client->getEnv().getClientMap().getNodeMetadata(loc.p);
		if (!meta || meta->getString("formspec").empty()) {
            formspec->quitMenu();
			return;
		}
	}

	if (isMenuActive())
        guiroot->bringToFront(formspec);
}

void GameFormSpec::disableDebugView()
{
    if (formspec) {
        formspec->setDebugView(false);
	}
}

/* returns false if game should exit, otherwise true
 */
bool GameFormSpec::handleCallbacks()
{
    auto cache = client->getResourceCache();

	if (g_gamecallback->disconnect_requested) {
		g_gamecallback->disconnect_requested = false;
		return false;
	}

	if (g_gamecallback->changepassword_requested) {
		(void)make_irr<GUIPasswordChange>(guienv, guiroot, -1,
                       &g_menumgr, client, cache);
		g_gamecallback->changepassword_requested = false;
	}

	if (g_gamecallback->changevolume_requested) {
		(void)make_irr<GUIVolumeChange>(guienv, guiroot, -1,
                     &g_menumgr, cache);
		g_gamecallback->changevolume_requested = false;
	}

	if (g_gamecallback->keyconfig_requested) {
		(void)make_irr<GUIKeyChangeMenu>(guienv, guiroot, -1,
                      &g_menumgr, cache);
		g_gamecallback->keyconfig_requested = false;
	}

	if (g_gamecallback->touchscreenlayout_requested) {
		(new GUITouchscreenLayout(guienv, guiroot, -1,
                     &g_menumgr, cache))->drop();
		g_gamecallback->touchscreenlayout_requested = false;
	}

	if (!g_gamecallback->show_open_url_dialog.empty()) {
		(void)make_irr<GUIOpenURLMenu>(guienv, guiroot, -1,
                 &g_menumgr, cache, g_gamecallback->show_open_url_dialog);
		g_gamecallback->show_open_url_dialog.clear();
	}

	if (g_gamecallback->keyconfig_changed) {
        input->keycache.populate(); // update the cache with new settings
		g_gamecallback->keyconfig_changed = false;
	}

	return true;
}

#ifdef __ANDROID__
bool GameFormSpec::handleAndroidUIInput()
{
	if (m_formspec) {
		m_formspec->getAndroidUIInput();
		return true;
	}
	return false;
}
#endif
