// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2019 Jordach, Jordan Snelling <jordach.snelling@gmail.com>

#pragma once

#include "Image/Converting.h"

struct SkyColor
{
    img::color8 day_sky;
    img::color8 day_horizon;
    img::color8 dawn_sky;
    img::color8 dawn_horizon;
    img::color8 night_sky;
	img::color8 night_horizon;
	img::color8 indoors;
};

struct SkyboxParams
{
	static constexpr float INVALID_SKYBOX_TILT = -1024.f;

	img::color8 bgcolor;
	std::string type;
	std::vector<std::string> textures;
	bool clouds;
	SkyColor sky_color;
	img::color8 fog_sun_tint;
	img::color8 fog_moon_tint;
	std::string fog_tint_type;
	float body_orbit_tilt { INVALID_SKYBOX_TILT };
	s16 fog_distance { -1 };
	float fog_start { -1.0f };
    img::color8 fog_color {}; // override, only used if alpha > 0
};

struct SunParams
{
	bool visible;
	std::string texture;
	std::string tonemap;
	std::string sunrise;
	bool sunrise_visible;
	f32 scale;
};

struct MoonParams
{
	bool visible;
	std::string texture;
	std::string tonemap;
	f32 scale;
};

struct StarParams
{
	bool visible;
	u32 count;
	img::color8 starcolor;
	f32 scale;
	f32 day_opacity;
};

struct CloudParams
{
	float density;
	img::color8 color_bright;
	img::color8 color_ambient;
	img::color8 color_shadow;
	float thickness;
	float height;
	v2f speed;
};

// Utility class for setting default sky, sun, moon, stars values:
class SkyboxDefaults
{
public:
	SkyboxDefaults() = delete;

	static const SkyboxParams getSkyDefaults()
	{
		SkyboxParams sky;
        sky.bgcolor = img::color8(img::PF_RGBA8, 255, 255, 255, 255);
		sky.type = "regular";
		sky.clouds = true;
		sky.sky_color = getSkyColorDefaults();
        sky.fog_sun_tint = img::color8(img::PF_RGBA8, 255, 244, 125, 29);
        sky.fog_moon_tint = colorfToColor8(img::colorf(img::PF_RGBA8, 0.5, 0.6, 0.8, 1));
		sky.fog_tint_type = "default";
        sky.fog_color = img::color8(img::PF_RGBA8);
		return sky;
	}

	static const SkyColor getSkyColorDefaults()
	{
		SkyColor sky;
		// Horizon colors
        sky.day_horizon = img::color8(img::PF_RGBA8, 255, 144, 211, 246);
        sky.indoors = img::color8(img::PF_RGBA8, 255, 100, 100, 100);
        sky.dawn_horizon = img::color8(img::PF_RGBA8, 255, 186, 193, 240);
        sky.night_horizon = img::color8(img::PF_RGBA8, 255, 64, 144, 255);
		// Sky colors
        sky.day_sky = img::color8(img::PF_RGBA8, 255, 97, 181, 245);
        sky.dawn_sky = img::color8(img::PF_RGBA8, 255, 180, 186, 250);
        sky.night_sky = img::color8(img::PF_RGBA8, 255, 0, 107, 255);
		return sky;
	}

	static const SunParams getSunDefaults()
	{
		SunParams sun;
		sun.visible = true;
		sun.sunrise_visible = true;
		sun.texture = "sun.png";
		sun.tonemap = "sun_tonemap.png";
		sun.sunrise = "sunrisebg.png";
		sun.scale = 1;
		return sun;
	}

	static const MoonParams getMoonDefaults()
	{
		MoonParams moon;
		moon.visible = true;
		moon.texture = "moon.png";
		moon.tonemap = "moon_tonemap.png";
		moon.scale = 1;
		return moon;
	}

	static const StarParams getStarDefaults()
	{
		StarParams stars;
		stars.visible = true;
		stars.count = 1000;
        stars.starcolor = img::color8(img::PF_RGBA8, 105, 235, 235, 255);
		stars.scale = 1;
		stars.day_opacity = 0;
		return stars;
	}

	static const CloudParams getCloudDefaults()
	{
		CloudParams clouds;
		clouds.density = 0.4f;
        clouds.color_bright = img::color8(img::PF_RGBA8, 229, 240, 240, 255);
        clouds.color_ambient = img::color8(img::PF_RGBA8, 255, 0, 0, 0);
        clouds.color_shadow = img::color8(img::PF_RGBA8, 255, 204, 204, 204);
		clouds.thickness = 16.0f;
		clouds.height = 120;
		clouds.speed = v2f(0.0f, -2.0f);
		return clouds;
	}
};
