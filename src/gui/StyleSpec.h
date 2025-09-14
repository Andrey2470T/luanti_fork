// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2019 rubenwardy

#pragma once

#include "client/media/resource.h"
#include "client/ui/glyph_atlas.h"
#include <Render/TTFont.h>
#include "debug.h"
#include "util/string.h"
#include <algorithm>
#include <array>

class StyleSpec
{
public:
	enum Property
	{
		TEXTCOLOR,
		BGCOLOR,
		BGCOLOR_HOVERED, // Note: Deprecated property
		BGCOLOR_PRESSED, // Note: Deprecated property
		NOCLIP,
		BORDER,
		BGIMG,
		BGIMG_HOVERED, // Note: Deprecated property
		BGIMG_MIDDLE,
		BGIMG_PRESSED, // Note: Deprecated property
		FGIMG,
		FGIMG_HOVERED, // Note: Deprecated property
		FGIMG_MIDDLE,
		FGIMG_PRESSED, // Note: Deprecated property
		ALPHA,
		CONTENT_OFFSET,
		PADDING,
		FONT,
		FONT_SIZE,
		COLORS,
		BORDERCOLORS,
		BORDERWIDTHS,
		SOUND,
		SPACING,
		SIZE,
		NUM_PROPERTIES,
		NONE
	};

	// State is a bitfield, it's possible to have multiple of these at once
	enum State : u8
	{
		STATE_DEFAULT = 0,
		STATE_FOCUSED = 1 << 0,
		STATE_HOVERED = 1 << 1,
		STATE_PRESSED = 1 << 2,
		NUM_STATES = 1 << 3, // This includes all permutations
		STATE_INVALID = 1 << 4,
	};

private:
	std::array<bool, NUM_PROPERTIES> property_set{};
	std::array<std::string, NUM_PROPERTIES> properties;
	State state_map = STATE_DEFAULT;

public:
	static Property GetPropertyByName(const std::string &name)
	{
		if (name == "textcolor") {
			return TEXTCOLOR;
		} else if (name == "bgcolor") {
			return BGCOLOR;
		} else if (name == "bgcolor_hovered") {
			return BGCOLOR_HOVERED;
		} else if (name == "bgcolor_pressed") {
			return BGCOLOR_PRESSED;
		} else if (name == "noclip") {
			return NOCLIP;
		} else if (name == "border") {
			return BORDER;
		} else if (name == "bgimg") {
			return BGIMG;
		} else if (name == "bgimg_hovered") {
			return BGIMG_HOVERED;
		} else if (name == "bgimg_middle") {
			return BGIMG_MIDDLE;
		} else if (name == "bgimg_pressed") {
			return BGIMG_PRESSED;
		} else if (name == "fgimg") {
			return FGIMG;
		} else if (name == "fgimg_hovered") {
			return FGIMG_HOVERED;
		} else if (name == "fgimg_middle") {
			return FGIMG_MIDDLE;
		} else if (name == "fgimg_pressed") {
			return FGIMG_PRESSED;
		} else if (name == "alpha") {
			return ALPHA;
		} else if (name == "content_offset") {
			return CONTENT_OFFSET;
		} else if (name == "padding") {
			return PADDING;
		} else if (name == "font") {
			return FONT;
		} else if (name == "font_size") {
			return FONT_SIZE;
		} else if (name == "colors") {
			return COLORS;
		} else if (name == "bordercolors") {
			return BORDERCOLORS;
		} else if (name == "borderwidths") {
			return BORDERWIDTHS;
		} else if (name == "sound") {
			return SOUND;
		} else if (name == "spacing") {
			return SPACING;
		} else if (name == "size") {
			return SIZE;
		} else {
			return NONE;
		}
	}

	std::string get(Property prop, std::string def) const
	{
		const auto &val = properties[prop];
		return val.empty() ? def : val;
	}

	void set(Property prop, const std::string &value)
	{
		properties[prop] = value;
		property_set[prop] = true;
	}

	//! Parses a name and returns the corresponding state enum
	static State getStateByName(const std::string &name)
	{
		if (name == "default") {
			return STATE_DEFAULT;
		} else if (name == "focused") {
			return STATE_FOCUSED;
		} else if (name == "hovered") {
			return STATE_HOVERED;
		} else if (name == "pressed") {
			return STATE_PRESSED;
		} else {
			return STATE_INVALID;
		}
	}

	//! Gets the state that this style is intended for
	State getState() const
	{
		return state_map;
	}

	//! Set the given state on this style
	void addState(State state)
	{
		FATAL_ERROR_IF(state >= NUM_STATES, "Out-of-bounds state received");

		state_map = static_cast<State>(state_map | state);
	}

	//! Using a list of styles mapped to state values, calculate the final
	//  combined style for a state by propagating values in its component states
	static StyleSpec getStyleFromStatePropagation(const std::array<StyleSpec, NUM_STATES> &styles, State state)
	{
		StyleSpec temp = styles[StyleSpec::STATE_DEFAULT];
		temp.state_map = state;
		for (int i = StyleSpec::STATE_DEFAULT + 1; i <= state; i++) {
			if ((state & i) != 0) {
				temp = temp | styles[i];
			}
		}

		return temp;
	}

	img::color8 getColor(Property prop, img::color8 def) const
	{
		const auto &val = properties[prop];
		if (val.empty()) {
			return def;
		}

		parseColorString(val, def, false, 0xFF);
		return def;
	}

	img::color8 getColor(Property prop) const
	{
		const auto &val = properties[prop];
		FATAL_ERROR_IF(val.empty(), "Unexpected missing property");

		img::color8 color;
		parseColorString(val, color, false, 0xFF);
		return color;
	}

	std::array<img::color8, 4> getColorArray(Property prop,
		std::array<img::color8, 4> def) const
	{
		const auto &val = properties[prop];
		if (val.empty())
			return def;

		std::vector<std::string> strs;
		if (!parseArray(val, strs))
			return def;

		for (size_t i = 0; i <= 3; i++) {
			img::color8 color;
			if (parseColorString(strs[i], color, false, 0xff))
				def[i] = color;
		}

		return def;
	}

	std::array<s32, 4> getIntArray(Property prop, std::array<s32, 4> def) const
	{
		const auto &val = properties[prop];
		if (val.empty())
			return def;

		std::vector<std::string> strs;
		if (!parseArray(val, strs))
			return def;

		for (size_t i = 0; i <= 3; i++)
			def[i] = stoi(strs[i]);

		return def;
	}

	recti getRect(Property prop, recti def) const
	{
		const auto &val = properties[prop];
		if (val.empty())
			return def;

		recti rect;
		if (!parseRect(val, &rect))
			return def;

		return rect;
	}

	recti getRect(Property prop) const
	{
		const auto &val = properties[prop];
		FATAL_ERROR_IF(val.empty(), "Unexpected missing property");

		recti rect;
		parseRect(val, &rect);
		return rect;
	}

    v2f getVector2f(Property prop, v2f def) const
	{
		const auto &val = properties[prop];
		if (val.empty())
			return def;

        v2f vec;
		if (!parseVector2f(val, &vec))
			return def;

		return vec;
	}

	v2i getVector2i(Property prop, v2i def) const
	{
		const auto &val = properties[prop];
		if (val.empty())
			return def;

        v2f vec;
		if (!parseVector2f(val, &vec))
			return def;

		return v2i(vec.X, vec.Y);
	}

	v2i getVector2i(Property prop) const
	{
		const auto &val = properties[prop];
		FATAL_ERROR_IF(val.empty(), "Unexpected missing property");

        v2f vec;
		parseVector2f(val, &vec);
		return v2i(vec.X, vec.Y);
	}

    render::TTFont *getFont(FontManager *font_mgr) const
	{
        render::FontMode mode = render::FontMode::GRAY;
        render::FontStyle style = render::FontStyle::NORMAL;

		const std::string &font = properties[FONT];
		const std::string &size = properties[FONT_SIZE];
        u32 font_size;

		if (font.empty() && size.empty())
			return nullptr;

		std::vector<std::string> modes = split(font, ',');

		for (size_t i = 0; i < modes.size(); i++) {
			if (modes[i] == "normal")
                style = render::FontStyle::NORMAL;
			else if (modes[i] == "mono")
                mode = render::FontMode::MONO;
			else if (modes[i] == "bold")
                style = render::FontStyle::BOLD;
			else if (modes[i] == "italic")
                style = render::FontStyle::ITALIC;
		}

		if (!size.empty()) {
			int calc_size = 1;

			if (size[0] == '*') {
				std::string new_size = size.substr(1); // Remove '*' (invalid for stof)
                calc_size = stof(new_size) * font_mgr->getDefaultFontSize(mode);
			} else if (size[0] == '+' || size[0] == '-') {
                calc_size = stoi(size) + font_mgr->getDefaultFontSize(mode);
			} else {
				calc_size = stoi(size);
			}

            font_size = (unsigned)std::min(std::max(calc_size, 1), 999);
		}

        return font_mgr->getFontOrCreate(mode, style, font_size);
	}

    img::Image *getTexture(Property prop, ResourceCache *cache,
			img::Image *def) const
	{
		const auto &val = properties[prop];
		if (val.empty()) {
			return def;
		}

        img::Image *texture = cache->getOrLoad<img::Image>(ResourceType::IMAGE, val);

		return texture;
	}

    img::Image *getTexture(Property prop, ResourceCache *cache) const
	{
		const auto &val = properties[prop];
		FATAL_ERROR_IF(val.empty(), "Unexpected missing property");

        img::Image *texture = cache->getOrLoad<img::Image>(ResourceType::IMAGE, val);

		return texture;
	}

	bool getBool(Property prop, bool def) const
	{
		const auto &val = properties[prop];
		if (val.empty()) {
			return def;
		}

		return is_yes(val);
	}

	inline bool isNotDefault(Property prop) const
	{
		return !properties[prop].empty();
	}

	inline bool hasProperty(Property prop) const { return property_set[prop]; }

	StyleSpec &operator|=(const StyleSpec &other)
	{
		for (size_t i = 0; i < NUM_PROPERTIES; i++) {
			auto prop = (Property)i;
			if (other.hasProperty(prop)) {
				set(prop, other.get(prop, ""));
			}
		}

		return *this;
	}

	StyleSpec operator|(const StyleSpec &other) const
	{
		StyleSpec newspec = *this;
		newspec |= other;
		return newspec;
	}

private:
	bool parseArray(const std::string &value, std::vector<std::string> &arr) const
	{
		std::vector<std::string> strs = split(value, ',');

		if (strs.size() == 1) {
			arr = {strs[0], strs[0], strs[0], strs[0]};
		} else if (strs.size() == 2) {
			arr = {strs[0], strs[1], strs[0], strs[1]};
		} else if (strs.size() == 4) {
			arr = strs;
		} else {
			warningstream << "Invalid array size (" << strs.size()
					<< " arguments): \"" << value << "\"" << std::endl;
			return false;
		}
		return true;
	}

	bool parseRect(const std::string &value, recti *parsed_rect) const
	{
		recti rect;
		std::vector<std::string> v_rect = split(value, ',');

		if (v_rect.size() == 1) {
			s32 x = stoi(v_rect[0]);
            rect.ULC = v2i(x, x);
            rect.LRC = v2i(-x, -x);
		} else if (v_rect.size() == 2) {
			s32 x = stoi(v_rect[0]);
			s32 y =	stoi(v_rect[1]);
            rect.ULC = v2i(x, y);
            rect.LRC = v2i(-x, -y);
			// `-x` is interpreted as `w - x`
		} else if (v_rect.size() == 4) {
            rect.ULC = v2i(
					stoi(v_rect[0]), stoi(v_rect[1]));
            rect.LRC = v2i(
					stoi(v_rect[2]), stoi(v_rect[3]));
		} else {
			warningstream << "Invalid rectangle string format: \"" << value
					<< "\"" << std::endl;
			return false;
		}

		*parsed_rect = rect;

		return true;
	}

    bool parseVector2f(const std::string &value, v2f *parsed_vec) const
	{
        v2f vec;
		std::vector<std::string> v_vector = split(value, ',');

		if (v_vector.size() == 1) {
			f32 x = stof(v_vector[0]);
			vec.X = x;
			vec.Y = x;
		} else if (v_vector.size() == 2) {
			vec.X = stof(v_vector[0]);
			vec.Y =	stof(v_vector[1]);
		} else {
			warningstream << "Invalid 2d vector string format: \"" << value
					<< "\"" << std::endl;
			return false;
		}

		*parsed_vec = vec;

		return true;
	}
};
