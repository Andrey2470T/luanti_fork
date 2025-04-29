#include "resource_loader.h"
#include "Image/ImageLoader.h"
#include "Image/ImageFilters.h"
#include "Render/Texture2D.h"
#include "Render/Shader.h"
#include "settings.h"
#include "file.h"
#include "log.h"

ResourceLoader::ResourceLoader(img::ImageModifier *_mdf)
	: mdf(_mdf)
{
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

	if (!img) {
		errorstream << "ResourceLoader: Couldn't load the image (" << path << ")" << std::endl;
		return nullptr;
	}

	return img::Align2Npot2(img, mdf);
}

render::Texture2D *ResourceLoader::loadTexture(const std::string &path)
{
    img::Image *img = loadImage(path);

	if (!img)
		return nullptr;
	
	fs::path name = fs::path(path).stem();

    return new render::Texture2D(name.string(), std::unique_ptr<img::Image>(img), render::TextureSettings());
}

render::Shader *ResourceLoader::loadShader(const std::string &path)
{
	std::ostringstream header;

	header << std::noboolalpha << std::showpoint;
	header << "#version 320\n";

	header << "#define ENABLE_WAVING_WATER " << (u8)enable_waving_water << "\n";
	if (enable_waving_water) {
		header << "#define WATER_WAVE_HEIGHT " << water_wave_height << "\n";
		header << "#define WATER_WAVE_LENGTH " << water_wave_length << "\n";
		header << "#define WATER_WAVE_SPEED " << water_wave_speed << "\n";
	}
	
	header << "#define ENABLE_WAVING_LEAVES " << (u8)enable_waving_leaves << "\n";
	header << "#define ENABLE_WAVING_PLANTS " << (u8)enable_waving_plants << "\n";
	
	header << "#define ENABLE_TONE_MAPPING " << (u8)tone_mapping << "\n";

    header << "#define ENABLE_DYNAMIC_SHADOWS " << (u8)enable_dynamic_shadows << "\n";
	if (enable_dynamic_shadows) {
		header << "#define COLORED_SHADOWS " << (u8)shadow_map_color << "\n";
		header << "#define POISSON_FILTER " << (u8)shadow_poisson_filter << "\n";
		header << "#define ENABLE_WATER_REFLECTIONS " << (u8)enable_water_reflections << "\n";
		header << "#define ENABLE_TRANSLUCENT_FOLIAGE " << (u8)enable_translucent_foliage << "\n";
		header << "#define ENABLE_NODE_SPECULAR " << (u8)enable_node_specular << "\n";

        header << "#define SHADOW_FILTER " << shadow_filters << "\n";
		header << "#define SOFTSHADOWRADIUS " << shadow_soft_radius << "\n";
	}

    header << "#define ENABLE_BLOOM " << (u8)enable_bloom << "\n";
	if (enable_bloom)
		header << "#define ENABLE_BLOOM_DEBUG " << (u8)enable_bloom_debug << "\n";

	header << "#define ENABLE_AUTO_EXPOSURE " << (u8)enable_auto_exposure << "\n";

	if (antialiasing == "ssaa") {
		header << "#define ENABLE_SSAA 1\n";
		header << "#define SSAA_SCALE " << fsaa << ".\n";
	}

	header << "#define ENABLE_DITHERING " << (u8)debanding << "\n";

	header << "#define VOLUMETRIC_LIGHT " << (u8)enable_volumetric_lighting << "\n";
	
	std::string final_header = "#line 0\n"; // reset the line counter for meaningful diagnostics

    std::string vs_code, fs_code, gs_code;
    File::read(fs::path(path) / "opengl_vertex.glsl", vs_code);
    File::read(fs::path(path) / "opengl_fragment.glsl", fs_code);
    File::read(fs::path(path) / "opengl_geometry.glsl", gs_code);

    std::string vertex_code = header.str() + final_header + vs_code;
    std::string fragment_code = header.str() + final_header + fs_code;
    std::string geometry_code;
	
    if (!gs_code.empty())
         geometry_code = header.str() + final_header + gs_code;

    return new render::Shader(vertex_code, fragment_code, geometry_code);
}

MeshBuffer *ResourceLoader::loadMesh(const std::string &path)
{}

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
    for (u32 i = 0; i < area; i++) {
        img::color8 c = mdf->getPixel(img, i % size.X, i / size.X);
        // Fill in palette with 'step' colors
        for (u32 j = 0; j < step; j++) {
            *colors_iter = c;
            colors_iter++;
        }
    }

    delete img;

    return new img::Palette(true, area, colors);
}
