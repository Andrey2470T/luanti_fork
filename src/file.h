#pragma once

#include "BasicIncludes.h"
#include "FilesystemVersions.h"
#include <string_view>

class File
{
public:
    static bool read(fs::path p, std::string &read_data);

    static bool write(fs::path p, std::string_view content, bool rewrite=false);
};
