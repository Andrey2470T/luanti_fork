#pragma once

#include "BasicIncludes.h"

namespace img
{
    class Image;
};

namespace render
{
    class Texture2D;
    class Shader;
};

class MeshBuffer;

class ResourceLoader
{
	bool enable_waving_water;
	f32 water_wave_height;
	f32 water_wave_length;
	f32 water_wave_speed;
	bool enable_waving_leaves;
	bool enable_waving_plants;
	bool tone_mapping;
	bool enable_dynamic_shadows;
	bool shadow_map_color;
	bool shadow_poisson_filter;
	bool enable_water_reflections;
	bool enable_translucent_foliage;
	bool enable_node_specular;
	s32 shadow_filters;
	f32 shadow_soft_radius;
	bool enable_bloom;
	bool enable_bloom_debug;
	bool enable_auto_exposure;
	std::string antialiasing;
	u16 fsaa;
	bool debanding;
	bool enable_volumetric_lighting;
public:
	ResourceLoader();

	img::Image *loadImage(const std::string &path);
	render::Texture2D *loadTexture(const std::string &path);
	render::Shader *loadShader(const std::string &path);
    MeshBuffer *loadMesh(const std::string &path);
	
};
