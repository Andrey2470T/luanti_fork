#include <fstream>
#include "file.h"
#include "log.h"

bool File::read(fs::path p, std::string &read_data)
{
    std::ifstream is(p);

    if (!is.is_open()) {
        errorstream << "File::read(): Failed to open " << p << std::endl;
        return false;
    }

    is.seekg(0, std::ios::end);
    u32 size = is.tellg();

    read_data.resize(size);

    is.seekg(0);
    is.read(&read_data[0], size);

    return true;
}

bool File::readLines(fs::path p, std::vector<std::string> &read_data)
{
    std::ifstream is(p);

    if (!is.is_open()) {
        errorstream << "File::readLines(): Failed to open " << p << std::endl;
        return false;
    }

    is.seekg(0);

    for (std::string line; std::getline(is, line, '\n');)
        read_data.push_back(line);

    return true;
}

bool File::write(fs::path p, std::string_view content, bool rewrite)
{
    std::ofstream os(p, rewrite ? std::ios::out : std::ios::app);

    if (!os.is_open()) {
        errorstream << "File::write(): Failed to open " << p << std::endl;
        return false;
    }

    os.write(&content[0], content.size());
    
    return true;
}
