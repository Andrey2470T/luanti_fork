// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2018 celeron55, Perttu Ahola <celeron55@gmail.com>

#pragma once

#include <BasicIncludes.h>
#include <deque>
#include <utility>
#include <Render/TTFont.h>
#include "profiler.h"

class RenderSystem;

/* Profiler display */
class ProfilerGraph
{
private:
	struct Piece
	{
		Piece(Profiler::GraphValues v) : values(std::move(v)) {}
		Profiler::GraphValues values;
	};
	struct Meta
	{
		float min;
		float max;
        img::color8 color;
		Meta(float initial = 0,
                img::color8 color = img::white) :
				min(initial),
				max(initial), color(color)
		{
		}
	};
	std::deque<Piece> m_log;

public:
	u32 m_log_max_size = 200;

	ProfilerGraph() = default;

	void put(const Profiler::GraphValues &values);

    void draw(s32 x_left, s32 y_bottom, RenderSystem *rndsys,
            render::TTFont *font) const;
};
