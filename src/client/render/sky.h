// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#pragma once

#include <BasicIncludes.h>
#include <Render/Texture2D.h>
#include <Image/Image.h>
#include <Render/Shader.h>
#include "client/player/playercamera.h" // CameraMode
#include "skyparams.h"

class RenderSystem;
class ResourceCache;
class Renderer;
class MeshBuffer;

#define SKY_MATERIAL_COUNT 12

class Sun
{
    RenderSystem *m_rndsys;
    SunParams m_sun_params;

    img::Image *m_image = nullptr;
    img::Image *m_tonemap = nullptr;
    img::Image *m_sunrise = nullptr;

    std::unique_ptr<MeshBuffer> m_mesh;
    std::unique_ptr<MeshBuffer> m_sunrise_mesh;
    bool m_first_update = true;

    matrix4 m_rotation;
    matrix4 m_sunrise_rotation;

    img::color8 m_base_color;
public:
    Sun(RenderSystem *rndsys, ResourceCache *cache);

    void setVisible(bool visible) { m_sun_params.visible = visible; }
    bool getVisible() const { return m_sun_params.visible; }
    void setImage(const std::string &image,
        const std::string &tonemap, ResourceCache *cache);
    void setScale(f32 scale) { m_sun_params.scale = scale; }
    void setSunriseVisible(bool glow_visible) { m_sun_params.sunrise_visible = glow_visible; }
    bool getSunriseVisible() const { return m_sun_params.sunrise_visible; }
    void setSunriseImage(const std::string &sunglow_texture, ResourceCache *cache);
    v3f getDirection(f32 time_of_day, f32 body_orbit_tilt);

    img::color8 getBaseColor() const
    {
        if (m_tonemap)
            return m_base_color;
        return img::black;
    }
    void draw();
    void drawSunrise();
    void update(const img::color8 &suncolor,
        const img::color8 &suncolor2, f32 time_of_day, float body_orbit_tilt);
    void updateSunrise(float time_of_day);
};

class Moon
{
    RenderSystem *m_rndsys;
    MoonParams m_moon_params;

    img::Image *m_image = nullptr;
    img::Image *m_tonemap = nullptr;

    std::unique_ptr<MeshBuffer> m_mesh;
    bool m_first_update = true;

    matrix4 m_rotation;

    img::color8 m_base_color;
public:
    Moon(RenderSystem *rndsys);

    void setVisible(bool visible) { m_moon_params.visible = visible; }
    bool getVisible() const { return m_moon_params.visible; }
    void setImage(const std::string &image,
        const std::string &tonemap, ResourceCache *cache);
    void setScale(f32 scale) { m_moon_params.scale = scale; }
    v3f getDirection(f32 time_of_day, f32 body_orbit_tilt);

    img::color8 getBaseColor() const
    {
        if (m_tonemap)
            return m_base_color;
        return img::black;
    }
    void draw();
    void update(const img::color8 &suncolor,
        const img::color8 &suncolor2, f32 time_of_day, float body_orbit_tilt);
};

class Stars
{
    RenderSystem *m_rndsys;
    StarParams m_star_params;
    u32 m_prev_star_count;

    std::unique_ptr<MeshBuffer> m_mesh;

    render::Shader *m_shader;

    bool m_first_update = true;

    matrix4 m_sky_rotation;

    u64 m_seed = 0;
public:
    Stars(RenderSystem *rndsys, ResourceCache *cache);

    void setVisible(bool visible) { m_star_params.visible = visible; }
    bool getVisible() const { return m_star_params.visible; }
    void setCount(u16 count);
    void setColor(img::color8 color) { m_star_params.starcolor = color; }
    void setScale(f32 scale) { m_star_params.scale = scale; updateMesh(); }
    void setDayOpacity(f32 day_opacity) { m_star_params.day_opacity = day_opacity; }

    void draw();
    void update(Renderer *rnd, float wicked_time_of_day, float body_orbit_tilt);
    void updateMesh();
};

// Skybox, rendered with zbuffer turned off, before all other nodes.
class Sky
{
public:
	//! constructor
    Sky(RenderSystem *rndsys, ResourceCache *cache);

    void render(PlayerCamera *camera);
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
    void addTextureToSkybox(const std::string &texture, u8 id);

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

    f32 getTimeOfDay() const
    {
        return m_time_of_day;
    }

    Sun *getSun() const
    {
        return m_sun.get();
    }
    Moon *getMoon() const
    {
        return m_moon.get();
    }
    Stars *getStars() const
    {
        return m_stars.get();
    }
    const SkyboxParams &getSkyParams() const
    {
        return m_sky_params;
    }

private:
    RenderSystem *m_rndsys;
    ResourceCache *m_cache;

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

    // update simple and far cloudy fogs
    void updateCloudyFogColor();

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

    img::colorf m_bgcolor_bright_f = img::colorf(1.0f, 1.0f, 1.0f, 1.0f);
    img::colorf m_skycolor_bright_f = img::colorf(1.0f, 1.0f, 1.0f, 1.0f);
    img::colorf m_cloudcolor_bright_f = img::colorf(1.0f, 1.0f, 1.0f, 1.0f);
    img::color8 m_bgcolor;
    img::color8 m_skycolor;
    img::colorf m_cloudcolor_f;

	// pure white: becomes "diffuse light component" for clouds
    img::colorf m_cloudcolor_day_f = img::colorf(1, 1, 1, 1);
	// dawn-factoring version of pure white (note: R is above 1.0)
    img::colorf m_cloudcolor_dawn_f = img::colorf(
		255.0f/240.0f,
		223.0f/240.0f,
		191.0f/255.0f
	);

    std::unique_ptr<Sun> m_sun;
    std::unique_ptr<Moon> m_moon;
    std::unique_ptr<Stars> m_stars;
	SkyboxParams m_sky_params;

    std::array<img::Image *, 6> m_skybox_images;

    std::unique_ptr<MeshBuffer> m_skybox_mesh;
    std::unique_ptr<MeshBuffer> m_cloudyfog_mesh;
    std::unique_ptr<MeshBuffer> m_far_cloudyfog_mesh;

    matrix4 m_world;

	bool m_default_tint = true;
};

// calculates value for sky body positions for the given observed time of day
// this is used to draw both Sun/Moon and shadows
float getWickedTimeOfDay(float time_of_day);
