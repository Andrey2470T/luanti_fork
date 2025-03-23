// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>
// Copyright (C) 2013 Jonathan Neuschäfer <j.neuschaefer@gmx.net>

#pragma once

#include <iostream>
#include <string_view>
#include "FilesystemVersions.h"

class FileCache
{
public:
	/*
		'dir' is the file cache directory to use.
	*/
	FileCache(const std::string &dir) : m_dir(dir) {}

	bool update(const std::string &name, std::string_view data);
	bool load(const std::string &name, std::ostream &os);
	bool exists(const std::string &name);

	// Copy another file on disk into the cache
	bool updateCopyFile(const std::string &name, const std::string &src_path);

private:
	fs::path m_dir;

	void createDir();
};
