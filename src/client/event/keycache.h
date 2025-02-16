#pragma once

#include "keypress.h"
#include "keytype.h"
#include <list>

class InputHandler;

/****************************************************************************
 Fast key cache for main game loop
 ****************************************************************************/

/* This is faster than using getKeySetting with the tradeoff that functions
 * using it must make sure that it's initialised before using it and there is
 * no error handling (for example bounds checking). This is really intended for
 * use only in the main running loop of the client (the_game()) where the faster
 * (up to 10x faster) key lookup is an asset. Other parts of the codebase
 * (e.g. formspecs) should continue using getKeySetting().
 */
struct KeyCache
{

	KeyCache()
	{
		handler = NULL;
		populate();
		populate_nonchanging();
	}

	void populate();

	// Keys that are not settings dependent
	void populate_nonchanging();

	KeyPress key[KeyType::INTERNAL_ENUM_COUNT];
	InputHandler *handler;
};

typedef std::list<KeyPress> super;
typedef super::iterator iter;
typedef super::const_iterator citer;

class KeyList : private std::list<KeyPress>
{
	virtual citer find(const KeyPress &key) const;

	virtual iter find(const KeyPress &key);

public:
	void clear() { super::clear(); }

	void set(const KeyPress &key)
	{
		if (find(key) == end())
			push_back(key);
	}

	void unset(const KeyPress &key)
	{
		iter p(find(key));

		if (p != end())
			erase(p);
	}

	void toggle(const KeyPress &key)
	{
		iter p(this->find(key));

		if (p != end())
			erase(p);
		else
			push_back(key);
	}

	void append(const KeyList &other)
	{
		for (const KeyPress &key : other) {
			set(key);
		}
	}

	bool operator[](const KeyPress &key) const { return find(key) != end(); }
};
