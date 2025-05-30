// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#pragma once

#include "exceptions.h"
#include "Main/Events.h"
#include <string>

class UnknownKeycode : public BaseException
{
public:
	UnknownKeycode(const char *s) :
		BaseException(s) {};
};

/* A key press, consisting of either an Irrlicht keycode
   or an actual char */

class KeyPress
{
public:
	KeyPress() = default;

	KeyPress(const char *name);

    KeyPress(const main::Event::KeyInputEvent &in, bool prefer_character = false);

	bool operator==(const KeyPress &o) const
	{
		return (Char > 0 && Char == o.Char) || (valid_kcode(Key) && Key == o.Key);
	}

	const char *sym() const;
	const char *name() const;

protected:
    static bool valid_kcode(main::KEY_CODE k)
	{
        return k > 0 && k < main::KEY_KEY_CODES_COUNT;
	}

    main::KEY_CODE Key = main::KEY_KEY_CODES_COUNT;
	wchar_t Char = L'\0';
	std::string m_name = "";
};

// Global defines for convenience

extern const KeyPress EscapeKey;

extern const KeyPress LMBKey;
extern const KeyPress MMBKey; // Middle Mouse Button
extern const KeyPress RMBKey;

// Key configuration getter
const KeyPress &getKeySetting(const char *settingname);

// Clear fast lookup cache
void clearKeyCache();

main::KEY_CODE keyname_to_keycode(const char *name);
