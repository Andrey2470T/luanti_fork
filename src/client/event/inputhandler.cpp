// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
// Copyright (C) 2017 nerzhul, Loic Blot <loic.blot@unix-experience.fr>

#include "settings.h"
#include "util/numeric.h"
#include "inputhandler.h"
#include "gui/mainmenumanager.h"
#include "gui/touchcontrols.h"
#include "hud.h"
#include "log_internal.h"

/*
 * RealInputHandler
 */
float RealInputHandler::getJoystickSpeed()
{
	if (g_touchcontrols && g_touchcontrols->getJoystickSpeed())
		return g_touchcontrols->getJoystickSpeed();
	return joystick.getMovementSpeed();
}

float RealInputHandler::getJoystickDirection()
{
	// `getJoystickDirection() == 0` means forward, so we cannot use
	// `getJoystickDirection()` as a condition.
	if (g_touchcontrols && g_touchcontrols->getJoystickSpeed())
		return g_touchcontrols->getJoystickDirection();
	return joystick.getMovementDirection();
}

v2i RealInputHandler::getMousePos()
{
    auto control = receiver->main_wnd->getCursorControl();
    return control.getPosition(false);
}

void RealInputHandler::setMousePos(s32 x, s32 y)
{
    auto &control = receiver->main_wnd->getCursorControl();
    control.setPosition({x, y});
}

/*
 * RandomInputHandler
 */
s32 RandomInputHandler::Rand(s32 min, s32 max)
{
	return (myrand() % (max - min + 1)) + min;
}

struct RandomInputHandlerSimData {
	std::string key;
	float counter;
	int time_max;
};

void RandomInputHandler::step(float dtime)
{
	static RandomInputHandlerSimData rnd_data[] = {
		{ "keymap_jump", 0.0f, 40 },
		{ "keymap_aux1", 0.0f, 40 },
		{ "keymap_forward", 0.0f, 40 },
		{ "keymap_left", 0.0f, 40 },
		{ "keymap_dig", 0.0f, 30 },
		{ "keymap_place", 0.0f, 15 }
	};

	for (auto &i : rnd_data) {
		i.counter -= dtime;
		if (i.counter < 0.0) {
			i.counter = 0.1 * Rand(1, i.time_max);
			keydown.toggle(getKeySetting(i.key.c_str()));
		}
	}
	{
		static float counter1 = 0;
		counter1 -= dtime;
		if (counter1 < 0.0) {
			counter1 = 0.1 * Rand(1, 20);
            mousespeed = v2i(Rand(-20, 20), Rand(-15, 20));
		}
	}
	mousepos += mousespeed;
	static bool useJoystick = false;
	{
		static float counterUseJoystick = 0;
		counterUseJoystick -= dtime;
		if (counterUseJoystick < 0.0) {
			counterUseJoystick = 5.0; // switch between joystick and keyboard direction input
			useJoystick = !useJoystick;
		}
	}
	if (useJoystick) {
		static float counterMovement = 0;
		counterMovement -= dtime;
		if (counterMovement < 0.0) {
			counterMovement = 0.1 * Rand(1, 40);
			joystickSpeed = Rand(0,100)*0.01;
			joystickDirection = Rand(-100, 100)*0.01 * M_PI;
		}
	} else {
		joystickSpeed = 0.0f;
		joystickDirection = 0.0f;
	}
}
