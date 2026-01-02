// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2018 celeron55, Perttu Ahola <celeron55@gmail.com>

#pragma once

#include <BasicIncludes.h>
#include <utility>
#include <list>
#include <Render/TTFont.h>
#include "profiler.h"
#include "extra_images.h"

class RenderSystem;
class ResourceCache;
class UIRects;
class UITextSprite;

/* Profiler display */
class ProfilerGraph
{
    RenderSystem *rndsys;

    std::list<f32> values;

    img::color8 color;

    u8 number;

    u32 log_max_size = 200;

    std::unique_ptr<UIRects> lines;
    std::unique_ptr<UITextSprite> text;
public:
    ProfilerGraph(RenderSystem *_rndsys, ResourceCache *cache, u8 _number, img::color8 _color = img::white);

    void update(const std::string &id, f32 new_value, s32 x_left, s32 y_bottom);

    void draw() const;
private:
    void updateText(const std::string &id, f32 show_min, f32 show_max,
        f32 ulc_x, f32 ulc_y, f32 lrc_x, f32 lrc_y);
};

class ProfilerGraphSet
{
    RenderSystem *rndsys;
    ResourceCache *cache;
    std::map<std::string, std::unique_ptr<ProfilerGraph>> graphs;

    bool is_visible = false;
public:
    ProfilerGraphSet(RenderSystem *_rndsys, ResourceCache *_cache)
        : rndsys(_rndsys), cache(_cache)
    {}

    void addGraph(const std::string &id, u8 number, img::color8 color = img::white)
    {
        graphs[id] = std::make_unique<ProfilerGraph>(rndsys, cache, number, color);
    }
    void put(const Profiler::GraphValues &values);

    void setVisible(bool visible)
    {
        is_visible = visible;
    }

    void draw() const;
};
