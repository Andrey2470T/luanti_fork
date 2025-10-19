#include "resource.h"
#include "client/render/renderer.h"
#include "porting.h"
#include "settings.h"
#include <Image/ImageLoader.h>
#include "client/mesh/model.h"
#include "client/render/atlas.h"
#include "client/mesh/layeredmesh.h"

std::vector<std::string> getTexturesDefaultPaths()
{
    std::vector<std::string> paths;

    fs::path texpath = g_settings->get("texture_path");

    if (fs::exists(texpath)) {
        for (auto &entry : fs::recursive_directory_iterator(texpath))
            paths.push_back(entry.path().string());
    }

    fs::path basePath = porting::path_share;
    basePath /= "textures";
    basePath /= "base";
    basePath /= "pack";

    paths.push_back(basePath.string());

    return paths;
}

std::vector<std::string> getShaderDefaultPaths()
{
    std::vector<std::string> paths;
    paths.push_back(g_settings->get("shader_path"));

    fs::path basePath = porting::path_share;
    basePath /= "client";
    basePath /= "shaders";

    paths.push_back(basePath.string());

    return paths;
}

std::vector<std::string> getNoPaths()
{
    std::vector<std::string> paths;
    return paths;
}

std::string texturePathFinder(const std::string &name)
{
    auto defpaths = getTexturesDefaultPaths();

    for (auto &p : defpaths) {
        fs::path fs_p(fs::path(p) / name);
        if (fs::exists(fs_p))
            return fs_p;
    }

    return "";
}

std::string shaderPathFinder(const std::string &name)
{
    auto defpaths = getShaderDefaultPaths();

    for (auto &p : defpaths) {
        fs::path vs_p(fs::path(p) / name / "opengl_vertex.glsl");
        fs::path fs_p(fs::path(p) / name / "opengl_fragment.glsl");

        if (fs::exists(vs_p) && fs::exists(fs_p))
            return fs::path(p) / name;
    }

    return "";
}

std::string fallbackPathFinder(const std::string &name)
{
    fs::path p = fs::absolute(name);
    fs::path fs_p = p / name;
    if (fs::exists(fs_p))
        return p;

    return "";
}

ResourceCache::ResourceCache()
    : loader(std::make_unique<ResourceLoader>()),
      texgen(std::make_unique<TextureGenerator>(this, g_imgmodifier))
{
    auto texDefPaths = getTexturesDefaultPaths();
    auto shaderDefPaths = getShaderDefaultPaths();
    auto noPaths = getNoPaths();

    ResourceLoader *resLoader = loader.get();

    std::function<std::string(const std::string&)> textureFinder = &texturePathFinder;
    std::function<img::Image*(const std::string&)> imageLoader = [resLoader](const std::string &name) -> img::Image* {
        return resLoader->loadImage(name);
    };

    std::function<render::Texture2D*(const std::string&)> textureLoader = [resLoader](const std::string &name) -> render::Texture2D* {
        return resLoader->loadTexture(name);
    };

    std::function<std::string(const std::string&)> shaderFinder = &shaderPathFinder;
    std::function<render::Shader*(const std::string&)> shaderLoader = [resLoader](const std::string &name) -> render::Shader* {
        return resLoader->loadShader(name);
    };

    std::function<std::string(const std::string&)> fallbackFinder = &fallbackPathFinder;
    std::function<img::Palette*(const std::string&)> paletteLoader = [resLoader](const std::string &name) -> img::Palette* {
        return resLoader->loadPalette(name);
    };

    subcaches[ResourceType::IMAGE] = std::make_unique<ResourceSubCache<img::Image>>(
        texDefPaths, textureFinder, imageLoader
    );
    subcaches[ResourceType::TEXTURE] = std::make_unique<ResourceSubCache<render::Texture2D>>(
        texDefPaths, textureFinder, textureLoader
    );
    subcaches[ResourceType::SHADER] = std::make_unique<ResourceSubCache<render::Shader>>(
        shaderDefPaths, shaderFinder, shaderLoader
    );
    subcaches[ResourceType::MODEL] = std::make_unique<ResourceSubCache<Model>>(
        noPaths, nullptr, nullptr
    );
    subcaches[ResourceType::PALETTE] = std::make_unique<ResourceSubCache<img::Palette>>(
        noPaths, fallbackFinder, paletteLoader
    );
    subcaches[ResourceType::ATLAS] = std::make_unique<ResourceSubCache<Atlas>>(
        noPaths, nullptr, nullptr
    );
    subcaches[ResourceType::FONT] = std::make_unique<ResourceSubCache<render::TTFont>>(
        noPaths, nullptr, nullptr
    );
}

