#include "resource_loader.h"
#include "Image/ImageLoader.h"
#include "FilesystemVersions.h"
#include "settings.h"
#include "filesys.h"

ResourceLoader::ResourceLoader()
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
	fsaa = std::max(2, g_settings->getU16("fsaa");
	debanding = g_settings->getBool("debanding");
	enable_volumetric_lighting = g_settings->getBool("enable_volumetric_lighting");
}

img::Image *ResourceLoader::loadImage(const std::string &path)
{
	return ImageLoader::load(path);
}

render::Texture2D *ResourceLoader::loadTexture(const std::string &path)
{
	img::Image *img = ImageLoader::load(path);
	
	fs::path name = fs::path(path).stem();
	return new render::Texture2D(name.string(), img, render::TextureSettings());
}

render::Shader *ResourceLoader::loadShader(const std::string &path)
{
	std::ostringstream header;

	header << std::noboolalpha << std::showpoint;
	header << "#version 320\n";

	header << "#define ENABLE_WAVING_WATER " << enable_waving_water << "\n";
	if (enable_waving_water) {
		header << "#define WATER_WAVE_HEIGHT " << water_wave_height << "\n";
		header << "#define WATER_WAVE_LENGTH " << water_wave_length << "\n";
		header << "#define WATER_WAVE_SPEED " << water_wave_speed << "\n";
	}
	
	header << "#define ENABLE_WAVING_LEAVES " << enable_waving_leaves << "\n";
	header << "#define ENABLE_WAVING_PLANTS " << enable_waving_plants << "\n";
	
	header << "#define ENABLE_TONE_MAPPING " << tone_mapping << "\n";

	if (enable_dynamic_shadows) {
		header << "#define ENABLE_DYNAMIC_SHADOWS 1\n";
		header << "#define COLORED_SHADOWS " << (u8)shadow_map_color << "\n";
		header << "#define POISSON_FILTER " << (u8)shadow_poisson_filter << "\n";
		header << "#define ENABLE_WATER_REFLECTIONS " << (u8)enable_water_reflections << "\n";
		header << "#define ENABLE_TRANSLUCENT_FOLIAGE " << (u8)enable_translucent_foliage << "\n";
		header << "#define ENABLE_NODE_SPECULAR " << (u8)enable_node_specular << "\n";

        header << "#define SHADOW_FILTER " << shadow_filters << "\n";
		header << "#define SOFTSHADOWRADIUS " << shadow_soft_radius << "\n";
	}

	if (enable_bloom) {
		header << "#define ENABLE_BLOOM 1\n";
		header << "#define ENABLE_BLOOM_DEBUG " << (u8)enable_bloom_debug << "\n";
	}

	header << "#define ENABLE_AUTO_EXPOSURE " << (u8)enable_auto_exposure << "\n";

	if (antialiasing == "ssaa") {
		header << "#define ENABLE_SSAA 1\n";
		header << "#define SSAA_SCALE " << fsaa << ".\n";
	}

	header << "#define ENABLE_DITHERING " << (u8)debanding << "\n";

	header << "#define VOLUMETRIC_LIGHT " << (u8)enable_volumetric_lighting << "\n";
	
	std::string final_header = "#line 0\n"; // reset the line counter for meaningful diagnostics

	std::string vertex_code = header.str() + final_header + File::read(fs::path(path / "opengl_vertex.glsl"));
	std::string fragment_code = header.str() + final_header + File::read(fs::path(path / "opengl_fragment.glsl"));
	std::string geometry_code = File::read(fs::path(path / "opengl_geometry.glsl"));
	
	if (!geometry_code.empty())
	     geometry_code = header.str() + final_header + geometry_code;

    return new render::Shader(vertex_code, fragment_code, geometry_code);
}

MeshBuffer *ResourceLoader::loadMesh(const std::strint &path)
{}