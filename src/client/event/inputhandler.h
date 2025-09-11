// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#pragma once

#include "joystickcontroller.h"
#include <list>
#include "eventreceiver.h"

class InputHandler
{
public:
    InputHandler(MyEventReceiver *_receiver)
        : receiver(_receiver)
	{
		keycache.handler = this;
		keycache.populate();
	}

	virtual ~InputHandler() = default;

	virtual bool isRandom() const
	{
		return false;
	}

	virtual bool isKeyDown(GameKeyType k) = 0;
	virtual bool wasKeyDown(GameKeyType k) = 0;
    virtual bool wasKeyPressed(GameKeyType k) = 0;
	virtual bool wasKeyReleased(GameKeyType k) = 0;
	virtual bool cancelPressed() = 0;

	virtual float getJoystickSpeed() = 0;
	virtual float getJoystickDirection() = 0;

    virtual void clearWasKeyPressed() {}
	virtual void clearWasKeyReleased() {}

    virtual void listenForKey(const MtKey &keyCode) {}
	virtual void dontListenForKeys() {}

    virtual v2i getMousePos() = 0;
	virtual void setMousePos(s32 x, s32 y) = 0;

	virtual s32 getMouseWheel() = 0;

	virtual void step(float dtime) {}

	virtual void clear() {}
	virtual void releaseAllKeys() {}

    MyEventReceiver *getReceiver() const
    {
        return receiver;
    }

	JoystickController joystick;
	KeyCache keycache;

    MyEventReceiver *receiver;
};

/*
	Separated input handler implementations
*/

class RealInputHandler final : public InputHandler
{
public:
    RealInputHandler(MyEventReceiver *receiver)
        : InputHandler(receiver)
	{
        receiver->joystick = &joystick;
	}

	virtual ~RealInputHandler()
	{
        receiver->joystick = nullptr;
	}

	virtual bool isKeyDown(GameKeyType k)
	{
        return receiver->IsKeyDown(keycache.key[k]) || joystick.isKeyDown(k);
	}
	virtual bool wasKeyDown(GameKeyType k)
	{
        return receiver->WasKeyDown(keycache.key[k]) || joystick.wasKeyDown(k);
	}
    virtual bool wasKeyPressed(GameKeyType k)
	{
        return receiver->WasKeyPressed(keycache.key[k]) || joystick.wasKeyPressed(k);
	}
	virtual bool wasKeyReleased(GameKeyType k)
	{
        return receiver->WasKeyReleased(keycache.key[k]) || joystick.wasKeyReleased(k);
	}

	virtual float getJoystickSpeed();

	virtual float getJoystickDirection();

	virtual bool cancelPressed()
	{
		return wasKeyDown(KeyType::ESC);
	}

    virtual void clearWasKeyPressed()
	{
        receiver->clearWasKeyPressed();
	}
	virtual void clearWasKeyReleased()
	{
        receiver->clearWasKeyReleased();
	}

    virtual void listenForKey(const MtKey &keyCode)
	{
        receiver->listenForKey(keyCode);
	}
	virtual void dontListenForKeys()
	{
        receiver->dontListenForKeys();
	}

    virtual v2i getMousePos();
	virtual void setMousePos(s32 x, s32 y);

	virtual s32 getMouseWheel()
	{
        return receiver->getMouseWheel();
	}

	void clear()
	{
		joystick.clear();
        receiver->clearInput();
	}

	void releaseAllKeys()
	{
		joystick.releaseAllKeys();
        receiver->releaseAllKeys();
	}
};

class RandomInputHandler final : public InputHandler
{
public:
    RandomInputHandler(MyEventReceiver *receiver)
        : InputHandler(receiver)
    {}

	bool isRandom() const
	{
		return true;
	}

	virtual bool isKeyDown(GameKeyType k) { return keydown[keycache.key[k]]; }
	virtual bool wasKeyDown(GameKeyType k) { return false; }
    virtual bool wasKeyPressed(GameKeyType k) { return false; }
	virtual bool wasKeyReleased(GameKeyType k) { return false; }
	virtual bool cancelPressed() { return false; }
	virtual float getJoystickSpeed() { return joystickSpeed; }
	virtual float getJoystickDirection() { return joystickDirection; }
    virtual v2i getMousePos() { return mousepos; }
    virtual void setMousePos(s32 x, s32 y) { mousepos = v2i(x, y); }

	virtual s32 getMouseWheel() { return 0; }

	virtual void step(float dtime);

	s32 Rand(s32 min, s32 max);

private:
	KeyList keydown;
    v2i mousepos;
    v2i mousespeed;
	float joystickSpeed;
	float joystickDirection;
};
