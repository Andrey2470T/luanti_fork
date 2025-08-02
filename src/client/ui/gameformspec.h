// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2024 cx384

#pragma once

#include <BasicIncludes.h>

class Client;
class RenderSystem;
class InputHandler;
class ISoundManager;
class GUIFormSpecMenu;

/*
This object intend to contain the core fromspec functionality.
It includes:
  - methods to show specific formspec menus
  - storing the opened fromspec
  - handling fromspec related callbacks
 */
struct GameFormSpec
{
    void init(Client *_client, RenderSystem *_rndsys, InputHandler *_input)
	{
        client = _client;
        rndsys = _rndsys;
        input = _input;
	}

	~GameFormSpec();

	void showFormSpec(const std::string &formspec, const std::string &formname);
	void showLocalFormSpec(const std::string &formspec, const std::string &formname);
	void showNodeFormspec(const std::string &formspec, const v3s16 &nodepos);
	void showPlayerInventory();
	void showDeathFormspecLegacy();
	void showPauseMenu();

	void update();
	void disableDebugView();

	bool handleCallbacks();

#ifdef __ANDROID__
	// Returns false if no formspec open
	bool handleAndroidUIInput();
#endif

private:
    Client *client;
    RenderSystem *rndsys;
    InputHandler *input;

	// Default: "". If other than "": Empty show_formspec packets will only
	// close the formspec when the formname matches
    std::string formname;
    GUIFormSpecMenu *formspec = nullptr;

	void deleteFormspec();
};
