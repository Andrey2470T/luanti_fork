#include "resource_loader.h"
#include "Image/ImageLoader.h"
#include "FilesystemVersions.h"
#include "settings.h"
#include "filesys.h"

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
	std::ostringstream header, 

	header << std::noboolalpha << std::showpoint;
	header << "#version 320\n";
    
    bool enable_waving_water = g_settings->getBool("enable_waving_water");
	header << "#define ENABLE_WAVING_WATER " << enable_waving_water << "\n";
	if (enable_waving_water) {
		header << "#define WATER_WAVE_HEIGHT " << g_settings->getFloat("water_wave_height") << "\n";
		header << "#define WATER_WAVE_LENGTH " << g_settings->getFloat("water_wave_length") << "\n";
		header << "#define WATER_WAVE_SPEED " << g_settings->getFloat("water_wave_speed") << "\n";
	}
	
	header << "#define ENABLE_WAVING_LEAVES " << g_settings->getBool("enable_waving_leaves") << "\n";
	header << "#define ENABLE_WAVING_PLANTS " << g_settings->getBool("enable_waving_plants") << "\n";
	
	header << "#define ENABLE_TONE_MAPPING " << g_settings->getBool("tone_mapping") << "\n";

	if (g_settings->getBool("enable_dynamic_shadows")) {
		header << "#define ENABLE_DYNAMIC_SHADOWS 1\n";
		if (g_settings->getBool("shadow_map_color"))
			header << "#define COLORED_SHADOWS 1\n";

		if (g_settings->getBool("shadow_poisson_filter"))
			header << "#define POISSON_FILTER 1\n";

		if (g_settings->getBool("enable_water_reflections"))
			header << "#define ENABLE_WATER_REFLECTIONS 1\n";

		if (g_settings->getBool("enable_translucent_foliage"))
			header << "#define ENABLE_TRANSLUCENT_FOLIAGE 1\n";

		if (g_settings->getBool("enable_node_specular"))
			header << "#define ENABLE_NODE_SPECULAR 1\n";

		s32 shadow_filter = g_settings->getS32("shadow_filters");
        header << "#define SHADOW_FILTER " << shadow_filter << "\n";

		f32 shadow_soft_radius = g_settings->getFloat("shadow_soft_radius");
		if (shadow_soft_radius < 1.0f)
			shadow_soft_radius = 1.0f;
		    header << "#define SOFTSHADOWRADIUS " << shadow_soft_radius << "\n";
	}

	if (g_settings->getBool("enable_bloom")) {
		header << "#define ENABLE_BLOOM 1\n";
		if (g_settings->getBool("enable_bloom_debug"))
			header << "#define ENABLE_BLOOM_DEBUG 1\n";
	}

	if (g_settings->getBool("enable_auto_exposure"))
		header << "#define ENABLE_AUTO_EXPOSURE 1\n";

	if (g_settings->get("antialiasing") == "ssaa") {
		header << "#define ENABLE_SSAA 1\n";
		u16 ssaa_scale = std::max(2, g_settings->getU16("fsaa"));
		header << "#define SSAA_SCALE " << ssaa_scale << ".\n";
	}

	if (g_settings->getBool("debanding"))
		header << "#define ENABLE_DITHERING 1\n";

	if (g_settings->getBool("enable_volumetric_lighting"))
		header << "#define VOLUMETRIC_LIGHT 1\n";
	
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