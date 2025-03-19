#pragma once

#include "BasicIncludes.h"

class File
{
    static std::string read(fs::path p);

    static void write(fs::path p, std::string content, bool rewrite=false);
};