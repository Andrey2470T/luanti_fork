// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>
// Copyright (C) 2013 Jonathan Neuschäfer <j.neuschaefer@gmx.net>

#include "filecache.h"

#include "log.h"
#include "file.h"
#include <string>
#include <iostream>
#include <fstream>
#include <cstdlib>

void FileCache::createDir()
{
    try {
        fs::create_directories(m_dir);
    }
    catch (const fs::filesystem_error &err) {
        errorstream << "FileCache::createDir(): " << err.what() << std::endl;
    }
}

bool FileCache::update(const std::string &name, std::string_view data)
{
    fs::path path = m_dir / name;
	return File::write(path, data, true);
}

bool FileCache::load(const std::string &name, std::ostream &os)
{
    fs::path path = m_dir / name;
    std::string data;
	bool status = File::read(path, data);
	
	if (status)
	    os << data;
	return status;
}

bool FileCache::exists(const std::string &name)
{
    fs::path path = m_dir / name;
    return fs::exists(path);
}

bool FileCache::updateCopyFile(const std::string &name, const std::string &src_path)
{
    fs::path path = m_dir / name;

	createDir();
    return fs::copy_file(src_path, path);
}
