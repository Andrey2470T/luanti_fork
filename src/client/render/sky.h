// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#pragma once

#include <BasicIncludes.h>
#include <Render/Texture2D.h>
#include <Image/Image.h>
#include "client/player/playercamera.h" // CameraMode
#include "skyparams.h"

class RenderSystem;
class ResourceCache;
class Renderer;
class MeshBuffer;

#define SKY_MATERIAL_COUNT 12

class Sun
{
    ResourceCache *m_cache;
    SunParams m_sun_params;

    render::Texture2D *m_texture = nullptr;
    img::Image *m_tonemap = nullptr;
public:
    Sun(ResourceCache *cache)
        : m_cache(cache)
    {}

    void setVisible(bool sun_visible) { m_sun_params.visible = sun_visible; }
    bool getVisible() const { return m_sun_params.visible; }
    void setTexture(const std::string &sun_texture,
        const std::string &sun_tonemap);
    void setScale(f32 sun_scale) { m_sun_params.scale = sun_scale; }
    void setSunriseVisible(bool glow_visible) { m_sun_params.sunrise_visible = glow_visible; }
    void setSunriseTexture(const std::string &sunglow_texture);
    v3f getSunDirection();

    void draw(Renderer *rnd, const img::color8 &suncolor,
        const img::color8 &suncolor2, float wicked_time_of_day);
};

class Moon
{
    ResourceCache *m_cache;
    MoonParams m_moon_params;

    render::Texture2D *m_texture = nullptr;
    img::Image *m_tonemap = nullptr;
public:
    Moon(ResourceCache *cache)
        : m_cache(cache)
    {}

    void setVisible(bool moon_visible) { m_moon_params.visible = moon_visible; }
    bool getVisible() const { return m_moon_params.visible; }
    void setTexture(const std::string &moon_texture,
        const std::string &moon_tonemap);
    void setMoonScale(f32 moon_scale) { m_moon_params.scale = moon_scale; }
    v3f getMoonDirection();

    void draw(Renderer *rnd, const img::color8 &mooncolor,
        const img::color8 &mooncolor2, float wicked_time_of_day);
};

class Stars
{
    ResourceCache *m_cache;
    StarParams m_star_params;

    std::unique_ptr<MeshBuffer> m_mesh;
public:
    Stars(ResourceCache *cache)
        : m_cache(cache)
    {}

    void setVisible(bool stars_visible) { m_star_params.visible = stars_visible; }
    void setCount(u16 star_count);
    void setColor(img::color8 star_color) { m_star_params.starcolor = star_color; }
    void setScale(f32 star_scale) { m_star_params.scale = star_scale; updateStars(); }
    void setDayOpacity(f32 day_opacity) { m_star_params.day_opacity = day_opacity; }

    void draw(Renderer *rnd, float wicked_time_of_day);
private:
    void updateStars();
};

// Skybox, rendered with zbuffer turned off, before all other nodes.
class Sky
{
public:
	//! constructor
    Sky(RenderSystem *rndsys, ResourceCache *cache);

    //virtual void OnRegisterSceneNode();

	//! renders the node.
    //virtual void render();

    //virtual const aabb3f &getBoundingBox() const { return m_box; }

	// Used by Irrlicht for optimizing rendering
    //virtual video::SMaterial &getMaterial(u32 i) { return m_materials[i]; }
    //virtual u32 getMaterialCount() const { return SKY_MATERIAL_COUNT; }

	void update(float m_time_of_day, float time_brightness, float direct_brightness,
			bool sunlight_seen, CameraMode cam_mode, float yaw, float pitch);

	float getBrightness() { return m_brightness; }

    img::color8 getBgColor() const
	{
		return m_visible ? m_bgcolor : m_fallback_bg_color;
	}

    img::color8 getSkyColor() const
	{
		return m_visible ? m_skycolor : m_fallback_bg_color;
	}

	bool getCloudsVisible() const { return m_clouds_visible && m_clouds_enabled; }
    const img::colorf &getCloudColor() const { return m_cloudcolor_f; }

	void setVisible(bool visible) { m_visible = visible; }

	// Set only from set_sky API
	void setCloudsEnabled(bool clouds_enabled) { m_clouds_enabled = clouds_enabled; }
    void setFallbackBgColor(img::color8 fallback_bg_color)
	{
		m_fallback_bg_color = fallback_bg_color;
	}
	void setBodyOrbitTilt(float body_orbit_tilt)
	{
		if (body_orbit_tilt != SkyboxParams::INVALID_SKYBOX_TILT)
			m_sky_params.body_orbit_tilt = rangelim(body_orbit_tilt, -90.f, 90.f);
	}
    void overrideColors(img::color8 bgcolor, img::color8 skycolor)
	{
		m_bgcolor = bgcolor;
		m_skycolor = skycolor;
	}
	void setSkyColors(const SkyColor &sky_color);
    void setHorizonTint(img::color8 sun_tint, img::color8 moon_tint,
		const std::string &use_sun_tint);
	void setInClouds(bool clouds) { m_in_clouds = clouds; }
	void clearSkyboxTextures() { m_sky_params.textures.clear(); }
    void addTextureToSkybox(const std::string &texture, int material_id);

	// Note: the Sky class doesn't use these values. It just stores them.
	void setFogDistance(s16 fog_distance) { m_sky_params.fog_distance = fog_distance; }
	s16 getFogDistance() const { return m_sky_params.fog_distance; }

	void setFogStart(float fog_start) { m_sky_params.fog_start = fog_start; }
	float getFogStart() const { return m_sky_params.fog_start; }

    void setFogColor(img::color8 v) { m_sky_params.fog_color = v; }
    img::color8 getFogColor() const {
        if (m_sky_params.fog_color.A() > 0)
			return m_sky_params.fog_color;
		return getBgColor();
	}

private:
    RenderSystem *m_rndsys;
    ResourceCache *m_cache;

    aabbf m_box{{0.0f, 0.0f, 0.0f}};
    //video::SMaterial m_materials[SKY_MATERIAL_COUNT];
	// How much sun & moon transition should affect horizon color
	float m_horizon_blend()
	{
		if (!m_sunlight_seen)
			return 0;
		float x = m_time_of_day >= 0.5 ? (1 - m_time_of_day) * 2
					       : m_time_of_day * 2;

		if (x <= 0.3)
			return 0;
		if (x <= 0.4) // when the sun and moon are aligned
			return (x - 0.3) * 10;
		if (x <= 0.5)
			return (0.5 - x) * 10;
		return 0;
	}

    // NOTE: BlendModes.h already has those
    // Mix two colors by a given amount
    /*static img::color8 m_mix_scolor(img::color8 col1, img::color8 col2, f32 factor)
	{
        img::color8 result = img::color8(
				col1.getAlpha() * (1 - factor) + col2.getAlpha() * factor,
				col1.getRed() * (1 - factor) + col2.getRed() * factor,
				col1.getGreen() * (1 - factor) + col2.getGreen() * factor,
				col1.getBlue() * (1 - factor) + col2.getBlue() * factor);
		return result;
	}
    static img::color8f m_mix_scolorf(img::color8f col1, img::color8f col2, f32 factor)
	{
        img::color8f result =
                img::color8f(col1.r * (1 - factor) + col2.r * factor,
						col1.g * (1 - factor) + col2.g * factor,
						col1.b * (1 - factor) + col2.b * factor,
						col1.a * (1 - factor) + col2.a * factor);
		return result;
    }*/

	bool m_visible = true;
	// Used when m_visible=false
    img::color8 m_fallback_bg_color = img::white;
	bool m_first_update = true; // Set before the sky is updated for the first time
	float m_time_of_day;
	float m_time_brightness;
	bool m_sunlight_seen;
	float m_brightness = 0.5f;
	float m_cloud_brightness = 0.5f;
	bool m_clouds_visible; // Whether clouds are disabled due to player underground
	bool m_clouds_enabled = true; // Initialised to true, reset only by set_sky API
	bool m_directional_colored_fog;
	bool m_in_clouds = true; // Prevent duplicating bools to remember old values

    img::colorf m_bgcolor_bright_f = img::colorf(img::PF_RGBA32F, 1.0f, 1.0f, 1.0f, 1.0f);
    img::colorf m_skycolor_bright_f = img::colorf(img::PF_RGBA32F, 1.0f, 1.0f, 1.0f, 1.0f);
    img::colorf m_cloudcolor_bright_f = img::colorf(img::PF_RGBA32F, 1.0f, 1.0f, 1.0f, 1.0f);
    img::color8 m_bgcolor;
    img::color8 m_skycolor;
    img::colorf m_cloudcolor_f;

	// pure white: becomes "diffuse light component" for clouds
    img::colorf m_cloudcolor_day_f = img::colorf(img::PF_RGBA32F, 1, 1, 1, 1);
	// dawn-factoring version of pure white (note: R is above 1.0)
    img::colorf m_cloudcolor_dawn_f = img::colorf(img::PF_RGBA32F,
		255.0f/240.0f,
		223.0f/240.0f,
		191.0f/255.0f
	);

	SkyboxParams m_sky_params;

	bool m_default_tint = true;

	u64 m_seed = 0;

	void draw_sky_body(std::array<video::S3DVertex, 4> &vertices,
        float pos_1, float pos_2, const img::color8 &c);
	void place_sky_body(std::array<video::S3DVertex, 4> &vertices,
		float horizon_position,	float day_position);
};

// calculates value for sky body positions for the given observed time of day
// this is used to draw both Sun/Moon and shadows
float getWickedTimeOfDay(float time_of_day);
