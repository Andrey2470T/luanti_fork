#include "resource.h"
#include "porting.h"
#include "settings.h"
#include "Image/ImageLoader.h"

static std::vector<std::string> getTexturesDefaultPaths()
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

std::string fontPathFinder(
    render::FontMode mode, render::FontStyle style, std::optional<u32> size
)
{
    std::array<u32, 3> defaultSizes = {
        std::clamp<u32>((u32)g_settings->getU16("mono_font_size"), 5, 72),
        std::clamp<u32>((u32)g_settings->getU16("font_size"), 5, 72),
        std::clamp<u32>((u32)g_settings->getU16("font_size"), 5, 72)
    };

    if (!size.has_value())
        size = defaultSizes[(u8)mode];

    u8 style8 = (u8)style;
    if (mode == render::FontMode::FALLBACK) {
        style8 &= ~(u8)render::FontStyle::BOLD;
        style8 &= ~(u8)render::FontStyle::ITALIC;
    }
    std::string prefix = "";

    if (mode == render::FontMode::MONO)
        prefix = "mono_";

    std::string suffix = "";

    if (style8 & (u8)render::FontStyle::BOLD)
        suffix = "_bold";
    if (style8 & (u8)render::FontStyle::ITALIC)
        suffix += "_italic";

    u16 divisible_by = g_settings->getU16(prefix + "font_size_divisible_by");
    if (divisible_by > 1) {
        size = std::max<u32>(
            std::round((f32)size.value() / divisible_by) * divisible_by, divisible_by);
    }
    
    u16 font_shadow  = 0;
	u16 font_shadow_alpha = 0;
	g_settings->getU16NoEx(prefix + "font_shadow", font_shadow);
	g_settings->getU16NoEx(prefix + "font_shadow_alpha",
			font_shadow_alpha);
			
    std::string path_setting;
	if (mode == render::FontMode::FALLBACK)
		path_setting = "fallback_font_path";
	else
		path_setting = prefix + "font_path" + suffix;

    std::string path1 = g_settings->get(path_setting);
    std::string path2 = Settings::getLayer(SL_DEFAULTS)->get(path_setting);
    
    if (fs::exists(path1))
        return path1;
    else if (fs::exists(path2))
        return path2;
    else
        return "";
}

static std::string fallbackPathFinder(const std::string &name)
{
    fs::path p = fs::absolute(name);
    fs::path fs_p = p / name;
    if (fs::exists(fs_p))
        return p;

    return "";
}

ResourceCache::ResourceCache(img::ImageModifier *mdf)
    : loader(std::make_unique<ResourceLoader>(mdf))
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
    meshes.reset(new ResourceSubCache<MeshBuffer>(
        {},
        &fallbackPathFinder,
        [resLoader] (const std::string &name) -> MeshBuffer*
        {
            return resLoader->loadMesh(name);
        }
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

