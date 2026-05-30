// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2021 x2048, Dmitry Kostenko <codeforsmile@gmail.com>

#pragma once
#include "Image/SColor.h"



/**
 * Parameters for automatic exposure compensation
 *
 * Automatic exposure compensation uses the following equation:
 *
 * wanted_exposure = 2^exposure_correction / clamp(observed_luminance, 2^luminance_min, 2^luminance_max)
 *
 */
struct AutoExposure
{
	/// @brief Minimum boundary for computed luminance
	float luminance_min{-3.0f};
	/// @brief Maximum boundary for computed luminance
	float luminance_max{-3.0f};
	/// @brief Luminance bias. Higher values make the scene darker, can be negative.
	float exposure_correction{0.0f};
	/// @brief Speed of transition from dark to bright scenes
	float speed_dark_bright{1000.0f};
	/// @brief Speed of transition from bright to dark scenes
	float speed_bright_dark{1000.0f};
	/// @brief Power value for center-weighted metering. Value of 1.0 measures entire screen uniformly
	float center_weight_power{1.0f};
};

// Parameters for four day time points (night, sunrise, day, sunset)
struct SkyColorTransitions
{
	video::SColor night {255, 10, 10, 31};
	video::SColor sunrise {255, 205, 90, 51};
	video::SColor day {255, 255, 251, 243};
	video::SColor sunset {255, 218, 77, 38};
};

// Describes global lighting settings for a player
struct Lighting
{
	SkyColorTransitions skycolors;
	video::SColor ambient_color {255, 38, 38, 38};

	AutoExposure exposure;
	float shadow_intensity {0.0f};
	float saturation {1.0f};
	float volumetric_light_strength {0.0f};
	video::SColor shadow_tint {255, 0, 0, 0};
	float bloom_intensity {0.05f};
	float bloom_strength_factor {1.0f};
	float bloom_radius {1.0f};
};
