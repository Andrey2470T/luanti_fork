// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#include "clientActiveObject.h"

/*
	ClientActiveObject
*/

ClientActiveObject::ClientActiveObject(u16 id, Client *client,
		ClientEnvironment *env):
	ActiveObject(id),
	m_client(client),
	m_env(env)
{}

