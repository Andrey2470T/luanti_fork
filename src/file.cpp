#include "file.h"

static std::string File::read(fs::path p)
{
    std::ifstream is(p);

    if (!is.is_open()) {
        errorstream << "File::read(): Failed to open " << p << std::endl;
        return "";
    }

    u32 size = is.tellg();

    std::string read_str;
    read_str.resize(size);

    is.seekg(0);
    is.read(&read_str[0], size);

    return read_str;
}

static void File::write(fs::path p, std::string content, bool rewrite)
{
    std::ofstream os(p, std::ios::out | (rewrite ? std::ios::out : std::ios::app));

    if (!os.is_open()) {
        errorstream << "File::write(): Failed to open " << p << std::endl;
        return;
    }

    os.write(&content, content.size());
}