#pragma once

#include "keycache.h"
#include <Main/MainWindow.h>
#include <Main/IEventReceiver.h>

class JoystickController;

enum class PointerType {
	Mouse,
	Touch,
};

using namespace main;

class MtEventReceiver : public IEventReceiver
{
public:
    MtEventReceiver(MainWindow *wnd)
        : main_wnd(wnd)
    {}

	// This is the one method that we have to implement
    virtual bool OnEvent(const Event &event);

    bool IsKeyDown(const MtKey &keyCode) const { return keyIsDown[keyCode]; }

	// Checks whether a key was down and resets the state
    bool WasKeyDown(const MtKey &keyCode)
	{
		bool b = keyWasDown[keyCode];
		if (b)
			keyWasDown.unset(keyCode);
		return b;
	}

	// Checks whether a key was just pressed. State will be cleared
	// in the subsequent iteration of Game::processPlayerInteraction
    bool WasMtKeyed(const MtKey &keycode) const { return keyWasPressed[keycode]; }

	// Checks whether a key was just released. State will be cleared
	// in the subsequent iteration of Game::processPlayerInteraction
    bool WasKeyReleased(const MtKey &keycode) const { return keyWasReleased[keycode]; }

    void listenForKey(const MtKey &keyCode)
	{
		keysListenedFor.set(keyCode);
	}
	void dontListenForKeys()
	{
		keysListenedFor.clear();
	}

	s32 getMouseWheel()
	{
		s32 a = mouse_wheel;
		mouse_wheel = 0;
		return a;
	}

	void clearInput()
	{
		keyIsDown.clear();
		keyWasDown.clear();
		keyWasPressed.clear();
		keyWasReleased.clear();

		mouse_wheel = 0;
	}

	void releaseAllKeys()
	{
		keyWasReleased.append(keyIsDown);
		keyIsDown.clear();
	}

    void clearWasMtKeyed()
	{
		keyWasPressed.clear();
	}

	void clearWasKeyReleased()
	{
		keyWasReleased.clear();
	}

	JoystickController *joystick = nullptr;

	PointerType getLastPointerType() { return last_pointer_type; }

    MainWindow *main_wnd;
private:

	s32 mouse_wheel = 0;

	// The current state of keys
	KeyList keyIsDown;

	// Like keyIsDown but only reset when that key is read
	KeyList keyWasDown;

	// Whether a key has just been pressed
	KeyList keyWasPressed;

	// Whether a key has just been released
	KeyList keyWasReleased;

	// List of keys we listen for
	// TODO perhaps the type of this is not really
	// performant as KeyList is designed for few but
	// often changing keys, and keysListenedFor is expected
	// to change seldomly but contain lots of keys.
	KeyList keysListenedFor;

	// Intentionally not reset by clearInput/releaseAllKeys.
	bool fullscreen_is_down = false;

	PointerType last_pointer_type = PointerType::Mouse;
};
