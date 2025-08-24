#include "resource.h"
#include "porting.h"
#include "settings.h"
#include "Image/ImageLoader.h"

std::vector<std::string> getTexturesDefaultPaths()
{
    std::vector<std::string> paths;
    for (auto &entry : fs::recursive_directory_iterator(g_settings->get("texture_path")))
        paths.push_back(entry.path().string());

    fs::path basePath = porting::path_share;
    basePath /= "textures";
    basePath /= "base";
    basePath /= "pack";

    paths.push_back(basePath.string());

    return paths;
}

static std::vector<std::string> getShaderDefaultPaths()
{
    std::vector<std::string> paths;
    paths.push_back(g_settings->get("shader_path"));

    fs::path basePath = porting::path_share;
    basePath /= "client";
    basePath /= "shaders";

    paths.push_back(basePath.string());

    return paths;
}

static std::string texturePathFinder(const std::string &name)
{
    auto defpaths = getTexturesDefaultPaths();

    for (auto &p : defpaths)
        for (auto &ext : img::SIE) {
            fs::path fs_p(fs::path(p) / (name + ext));
            if (fs::exists(fs_p))
                return p;
        }

    return "";
}

static std::string shaderPathFinder(const std::string &name)
{
    auto defpaths = getShaderDefaultPaths();

    for (auto &p : defpaths) {
        fs::path vs_p(fs::path(p) / name / "opengl_vertex.glsl");
        fs::path fs_p(fs::path(p) / name / "opengl_fragment.glsl");

        if (fs::exists(vs_p) && fs::exists(fs_p))
            return p;
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
    : loader(std::make_unique<ResourceLoader>())
{
    auto texDefPaths = getTexturesDefaultPaths();
    auto shaderDefPaths = getShaderDefaultPaths();

    ResourceLoader *resLoader = loader.get();
    images.reset(new ResourceSubCache<img::Image>(
        texDefPaths,
        &texturePathFinder,
        [resLoader] (const std::string &name) -> img::Image*
        {
            return resLoader->loadImage(name);
        }
    ));
    textures.reset(new ResourceSubCache<render::Texture2D>(
        texDefPaths,
        &texturePathFinder,
        [resLoader] (const std::string &name) -> render::Texture2D*
        {
            return resLoader->loadTexture(name);
        }
    ));
    shaders.reset(new ResourceSubCache<render::Shader>(
        shaderDefPaths,
        &shaderPathFinder,
        [resLoader] (const std::string &name) -> render::Shader*
        {
            return resLoader->loadShader(name);
        }
    ));
    models.reset(new ResourceSubCache<Model>(
        {},
        nullptr,
        nullptr
    ));
    palettes.reset(new ResourceSubCache<img::Palette>(
        {},
        &fallbackPathFinder,
        [resLoader] (const std::string &name) -> img::Palette*
        {
            return resLoader->loadPalette(name);
        }
    ));
    atlases.reset(new ResourceSubCache<Atlas>(
        {},
        nullptr,
        nullptr
    ));
    fonts.reset(new ResourceSubCache<render::TTFont>(
        {},
        nullptr,
        nullptr
     ));
}

