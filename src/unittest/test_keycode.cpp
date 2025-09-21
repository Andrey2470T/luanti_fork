// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2016 sfan5 <sfan5@live.de>

#include "test.h"

#include <string>
#include "exceptions.h"
#include "client/event/keypress.h"
#include <Core/Events.h>

class TestKeycode : public TestBase {
public:
	TestKeycode() { TestManager::registerTestModule(this); }
	const char *getName() { return "TestKeycode"; }

	void runTests(IGameDef *gamedef);

	void testCreateFromString();
	void testCreateFromSKeyInput();
	void testCompare();
};

static TestKeycode g_test_instance;

void TestKeycode::runTests(IGameDef *gamedef)
{
	TEST(testCreateFromString);
	TEST(testCreateFromSKeyInput);
	TEST(testCompare);
}

////////////////////////////////////////////////////////////////////////////////

#define UASSERTEQ_STR(one, two) UASSERT(strcmp(one, two) == 0)

void TestKeycode::testCreateFromString()
{
    MtKey k;

	// Character key, from char
    k = MtKey("R");
	UASSERTEQ_STR(k.sym(), "KEY_KEY_R");
	UASSERTCMP(int, >, strlen(k.name()), 0); // should have human description

	// Character key, from identifier
    k = MtKey("KEY_KEY_B");
	UASSERTEQ_STR(k.sym(), "KEY_KEY_B");
	UASSERTCMP(int, >, strlen(k.name()), 0);

	// Non-Character key, from identifier
    k = MtKey("KEY_UP");
	UASSERTEQ_STR(k.sym(), "KEY_UP");
	UASSERTCMP(int, >, strlen(k.name()), 0);

    k = MtKey("KEY_F6");
	UASSERTEQ_STR(k.sym(), "KEY_F6");
	UASSERTCMP(int, >, strlen(k.name()), 0);

	// Irrlicht-unknown key, from char
    k = MtKey("/");
	UASSERTEQ_STR(k.sym(), "/");
	UASSERTCMP(int, >, strlen(k.name()), 0);
}

void TestKeycode::testCreateFromSKeyInput()
{
    MtKey k;
    core::Event::KeyInputEvent in;

	// Character key
    in.Key = KEY_KEY_3;
	in.Char = L'3';
    k = MtKey(in);
	UASSERTEQ_STR(k.sym(), "KEY_KEY_3");

	// Non-Character key
    in.Key = KEY_RSHIFT;
	in.Char = L'\0';
    k = MtKey(in);
	UASSERTEQ_STR(k.sym(), "KEY_RSHIFT");

	// Irrlicht-unknown key
    in.Key = KEY_KEY_CODES_COUNT;
	in.Char = L'?';
    k = MtKey(in);
	UASSERTEQ_STR(k.sym(), "?");

	// prefer_character mode
    in.Key = KEY_COMMA;
	in.Char = L'G';
    k = MtKey(in, true);
	UASSERTEQ_STR(k.sym(), "KEY_KEY_G");
}

void TestKeycode::testCompare()
{
	// Basic comparison
    UASSERT(MtKey("5") == MtKey("KEY_KEY_5"));
    UASSERT(!(MtKey("5") == MtKey("KEY_NUMPAD5")));

	// Matching char suffices
	// note: This is a real-world example, Irrlicht maps XK_equal to irr::KEY_PLUS on Linux
    core::Event::KeyInputEvent in;
    in.Key = KEY_PLUS;
	in.Char = L'=';
    UASSERT(MtKey("=") == MtKey(in));

	// Matching keycode suffices
    core::Event::KeyInputEvent in2;
    in.Key = in2.Key = KEY_OEM_CLEAR;
	in.Char = L'\0';
	in2.Char = L';';
    UASSERT(MtKey(in) == MtKey(in2));
}
