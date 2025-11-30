#include "resource_loader.h"
#include <Image/ImageLoader.h>
#include <Render/Texture2D.h>
#include <Render/Shader.h>
#include "client/render/renderer.h"
#include "client/mesh/model.h"
#include "settings.h"
#include "file.h"
#include "log.h"
#include "porting.h"
#include <sstream>

ResourceLoader::ResourceLoader()
{
    enableGUIFiltering = g_settings->getBool("gui_scaling_filter");

	enable_waving_water = g_settings->getBool("enable_waving_water");
	water_wave_height = g_settings->getFloat("water_wave_height");
	water_wave_length = g_settings->getFloat("water_wave_length");
	water_wave_speed = g_settings->getFloat("water_wave_speed");
	enable_waving_leaves = g_settings->getBool("enable_waving_leaves");
	enable_waving_plants = g_settings->getBool("enable_waving_plants");
	tone_mapping = g_settings->getBool("tone_mapping");
	enable_dynamic_shadows = g_settings->getBool("enable_dynamic_shadows");
	shadow_map_color = g_settings->getBool("shadow_map_color");
	shadow_poisson_filter = g_settings->getBool("shadow_poisson_filter");
	enable_water_reflections = g_settings->getBool("enable_water_reflections");
	enable_translucent_foliage = g_settings->getBool("enable_translucent_foliage");
	enable_node_specular = g_settings->getBool("enable_node_specular");
	shadow_filters = g_settings->getS32("shadow_filters");
	shadow_soft_radius = std::max(1.0f, g_settings->getFloat("shadow_soft_radius"));
	enable_bloom = g_settings->getBool("enable_bloom");
	enable_bloom_debug = g_settings->getBool("enable_bloom_debug");
	enable_auto_exposure = g_settings->getBool("enable_auto_exposure");
	antialiasing = g_settings->get("antialiasing");
    fsaa = std::max((u16)2, g_settings->getU16("fsaa"));
	debanding = g_settings->getBool("debanding");
	enable_volumetric_lighting = g_settings->getBool("enable_volumetric_lighting");
}

img::Image *ResourceLoader::loadImage(const std::string &path)
{
    img::Image *img = img::ImageLoader::load(path);
    //fs::path p = fs::path(path).filename();
    //if (img)
        //img::ImageLoader::save(img, "/home/andrey/minetests/luanti_fork/cache/atlases/" + p.string());

	if (!img) {
		errorstream << "ResourceLoader: Couldn't load the image (" << path << ")" << std::endl;
		return nullptr;
	}

    if (enableGUIFiltering) {
        auto newImg = g_imgmodifier->copyWith2NPot2Scaling(img);
        delete img;
        img = newImg;
    }
    return img;
}

render::Texture2D *ResourceLoader::loadTexture(const std::string &path)
{
    img::Image *img = loadImage(path);

	if (!img)
		return nullptr;
	
    fs::path name = fs::path(path).filename();

    return new render::Texture2D(name.string(), std::unique_ptr<img::Image>(img), render::TextureSettings());
}

std::string getIncludePath(const std::string &name)
{
    std::vector<std::string> paths;
    paths.push_back(g_settings->get("shader_path"));

    fs::path basePath = porting::path_share;
    basePath /= "client";
    basePath /= "shaders";

    paths.push_back(basePath.string());

    for (auto &p : paths) {
        std::string fullname = name + ".glsl";
        fs::path fullpath(fs::path(p) / "includes" / fullname);

        if (fs::exists(fullpath))
            return fullpath.string();
    }

    return "";
}

#define PUT_CONSTANT(str, name, value) \
    str << "#define " << name << " " << value << "\n";

render::Shader *ResourceLoader::loadShader(const std::string &path)
{
    std::ostringstream header;

    header << std::noboolalpha << std::showpoint;

    header << "#version 330 core\n";

    header << "#define ENABLE_WAVING_WATER " << (enable_waving_water ? 1 : 0) << "\n";
	if (enable_waving_water) {
        header << "#define WATER_WAVE_HEIGHT " << water_wave_height << "\n";
        header << "#define WATER_WAVE_LENGTH " << water_wave_length << "\n";
        header << "#define WATER_WAVE_SPEED " << water_wave_speed << "\n";
	}

    header << "#define ENABLE_WAVING_LEAVES " << (enable_waving_leaves ? 1 : 0) << "\n";
    header << "#define ENABLE_WAVING_PLANTS " << (enable_waving_plants ? 1 : 0) << "\n";

    if (tone_mapping) header << "#define ENABLE_TONE_MAPPING 1\n";

	if (enable_dynamic_shadows) {
        header << "#define ENABLE_DYNAMIC_SHADOWS 1\n";
        if (shadow_map_color)
            header << "#define COLORED_SHADOWS 1\n";
        if (shadow_poisson_filter)
            header << "#define POISSON_FILTER 1\n";
        if (enable_water_reflections)
            header << "#define ENABLE_WATER_REFLECTIONS 1\n";
        if (enable_translucent_foliage)
            header << "#define ENABLE_TRANSLUCENT_FOLIAGE 1\n";
        if (enable_node_specular)
            header << "#define ENABLE_NODE_SPECULAR 1\n";

        header << "#define SHADOW_FILTER " << shadow_filters << "\n";
		header << "#define SOFTSHADOWRADIUS " << shadow_soft_radius << "\n";
	}

    if (enable_bloom) {
        header << "#define ENABLE_BLOOM 1\n";
		header << "#define ENABLE_BLOOM_DEBUG " << (enable_bloom_debug ? 1 : 0) << "\n";
    }

    if (enable_auto_exposure) header << "#define ENABLE_AUTO_EXPOSURE 1\n";

	if (antialiasing == "ssaa") {
		header << "#define ENABLE_SSAA 1\n";
		header << "#define SSAA_SCALE " << fsaa << ".\n";
	}

    if (debanding)
        header << "#define ENABLE_DITHERING 1\n";
    if (enable_volumetric_lighting)
        header << "#define VOLUMETRIC_LIGHT 1\n";

    header << "#define TILE_MATERIAL_BASIC 0\n";
    header << "#define TILE_MATERIAL_ALPHA 1\n";
    header << "#define TILE_MATERIAL_LIQUID_TRANSPARENT 2\n";
    header << "#define TILE_MATERIAL_LIQUID_OPAQUE 3\n";
    header << "#define TILE_MATERIAL_WAVING_LEAVES 4\n";
    header << "#define TILE_MATERIAL_WAVING_PLANTS 5\n";
    header << "#define TILE_MATERIAL_OPAQUE 6\n";
    header << "#define TILE_MATERIAL_WAVING_LIQUID_BASIC 7\n";
    header << "#define TILE_MATERIAL_WAVING_LIQUID_TRANSPARENT 8\n";
    header << "#define TILE_MATERIAL_WAVING_LIQUID_OPAQUE 9\n";
    // Note: PLAIN isn't a material actually used by tiles, rather just entities.
    header << "#define TILE_MATERIAL_PLAIN 10\n";
    header << "#define TILE_MATERIAL_PLAIN_ALPHA 11\n";

	std::string final_header = "#line 0\n";

    std::string vs_code, fs_code, gs_code;
    vs_code = parseShader(path, "vertex");
    fs_code = parseShader(path, "fragment");
    gs_code = parseShader(path, "geometry");

    if (vs_code.empty() || fs_code.empty())
        return nullptr;

    std::string vertex_code = header.str() + final_header + vs_code;
    std::string fragment_code = header.str() + final_header + fs_code;
    std::string geometry_code;
	
    if (!gs_code.empty())
         geometry_code = header.str() + final_header + gs_code;

    return new render::Shader(vertex_code, fragment_code, geometry_code);
}

img::Palette *ResourceLoader::loadPalette(const std::string &path)
{
	img::Image *img = loadImage(path);

    if (!img)
        return nullptr;

    v2u size = img->getSize();
    u32 area = size.X * size.Y;

    if (area > 256) {
        warningstream << "ResourceLoader::loadPalette(): the specified"
            << " palette image \"" << fs::path().stem() << "\" is larger than 256"
            << " pixels, using the first 256." << std::endl;
        area = 256;
    }
    else if (256 % area != 0) {
        warningstream << "ResourceLoader::loadPalette(): the "
                      << "specified palette image \"" << fs::path().stem() << "\" does not "
            << "contain power of two pixels." << std::endl;
    }

    std::vector<img::color8> colors(256);
    std::vector<img::color8>::iterator colors_iter = colors.begin();

    // We stretch the palette so it will fit 256 values
    // This many param2 values will have the same color
    u32 step = 256 / area;
    // For each pixel in the image
    img::color8 c;
    for (u32 i = 0; i < area; i++) {
        g_imgmodifier->getPixelDirect(img, i % size.X, i / size.X, c);
        // Fill in palette with 'step' colors
        for (u32 j = 0; j < step; j++) {
            *colors_iter = c;
            colors_iter++;
        }
    }

    delete img;

    return new img::Palette(true, area, colors);
}

std::string ResourceLoader::parseShader(const std::string &path, const std::string &type)
{
    std::vector<std::string> lines;

    std::string fullname = "opengl_" + type + ".glsl";
    fs::path fullpath(path);
    fullpath /= fullname;

    if (type == "geometry" && !fs::exists(fullpath))
        return "";

    File::readLines(fullpath, lines);

    if (lines.empty()) {
        errorstream << "ResourceLoader::loadShader(): failed to compile " << type << " shader: no code provided" << std::endl;
        return "";
    }

    std::string res_code;

    for (auto &line : lines) {
        if (str_starts_with(line, "#include")) {
            auto s = line.find("<");
            auto e = line.find(">");

            if (s == std::string::npos || e == std::string::npos)
                continue;
            s++;

            auto filename = line.substr(s, e-s);
            auto include_p = getIncludePath(filename);

            std::string include_code;
            bool read_success = File::read(include_p, include_code);

            if (read_success) {
                res_code += include_code;
                res_code += "\n";
            }
        }
        else {
            res_code += line;
            res_code += "\n";
        }
    }

    return res_code;
}
