// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
// Copyright (C) 2018 nerzhul, Loic Blot <loic.blot@unix-experience.fr>

#include "client/render/renderer.h"
#include "gui/IGUIEnvironment.h"
#include "porting.h"
#include "profilergraph.h"
#include "client/render/rendersystem.h"
#include "client/mesh/meshbuffer.h"
#include "client/mesh/defaultVertexTypes.h"
#include "text_sprite.h"
#include "glyph_atlas.h"
#include "batcher2d.h"
#include "util/string.h"

ProfilerGraph::ProfilerGraph(RenderSystem *_rndsys, ResourceCache *cache, u8 _number, img::color8 _color)
    : rndsys(_rndsys), color(_color), number(_number),
      lines(std::make_unique<MeshBuffer>(false, VType2D, render::MeshUsage::DYNAMIC)),
      text(std::make_unique<UITextSprite>(rndsys->getFontManager(), rndsys->getGUIEnvironment()->getSkin(),
            EnrichedString(), rndsys->getRenderer(), cache))
{
    text->setOverrideFont(rndsys->getFontManager()->getDefaultFont());
    text->enableWordWrap(true);
}

void ProfilerGraph::update(const std::string &id, f32 new_value, s32 x_left, s32 y_bottom)
{
    values.emplace_back(new_value);

    while (values.size() > log_max_size)
        values.erase(values.begin());

    for (f32 v : values) {
        min_value = std::min(min_value, v);
        max_value = std::max(max_value, v);
    }

    s32 graphh = 50;
    s32 textx = x_left + log_max_size + 15;
    s32 textx2 = textx + 200 - 15;

    s32 x = x_left;
    s32 y = y_bottom - number * 50;

    f32 show_min = min_value;
    f32 show_max = max_value;

    if (show_min >= -0.0001 && show_max >= -0.0001) {
        if (show_min <= show_max * 0.5)
            show_min = 0;
    }

    updateText(id, show_min, show_max, textx, y - graphh, textx2, y);

    s32 graph1y = y;
    s32 graph1h = graphh;
    bool relativegraph = (show_min != 0 && show_min != show_max);
    f32 lastscaledvalue = 0.0;


    lines->clear();

    for (f32 v : values) {
        float scaledvalue = 1.0;

        if (show_max != show_min)
            scaledvalue = (v - show_min) / (show_max - show_min);

        if (scaledvalue == 1.0 && v == 0) {
            x++;
            continue;
        }

        if (relativegraph) {
            s32 ivalue1 = lastscaledvalue * graph1h;
            s32 ivalue2 = scaledvalue * graph1h;

            v2f start_pos(x - 1, graph1y - ivalue1);
            v2f end_pos(x, graph1y - ivalue2);

            Batcher2D::appendTriangle(lines.get(), {start_pos, end_pos, end_pos}, color);
            lastscaledvalue = scaledvalue;
        } else {
            s32 ivalue = scaledvalue * graph1h;

            v2f start_pos(x, graph1y);
            v2f end_pos(x, graph1y - ivalue);

            Batcher2D::appendTriangle(lines.get(), {start_pos, end_pos, end_pos}, color);
        }

        x++;
    }

    lines->uploadData();
}

void ProfilerGraph::draw() const
{
    auto rnd = rndsys->getRenderer();

    rnd->setDefaultShader(true, true);
    rnd->setDefaultUniforms(1.0f, 1, 0.5f, img::BM_COUNT);

    rnd->draw(lines.get());

    text->draw();
}

void ProfilerGraph::updateText(const std::string &id, f32 show_min, f32 show_max,
    f32 ulc_x, f32 ulc_y, f32 lrc_x, f32 lrc_y)
{
    std::wstring text_str;

    char buf[20];
    if (floorf(show_max) == show_max)
        porting::mt_snprintf(buf, sizeof(buf), "%.5g", show_max);
    else
        porting::mt_snprintf(buf, sizeof(buf), "%.3g", show_max);

    text_str = utf8_to_wide(buf);
    text_str += L"\n";
    text_str += utf8_to_wide(id);
    text_str += L"\n";

    if (floorf(show_min) == show_min)
        porting::mt_snprintf(buf, sizeof(buf), "%.5g", show_min);
    else
        porting::mt_snprintf(buf, sizeof(buf), "%.3g", show_min);

    text_str += utf8_to_wide(buf);

    text->setText(text_str);
    text->updateBuffer(rectf(ulc_x, ulc_y, lrc_x, lrc_y));
}

void ProfilerGraphSet::put(const Profiler::GraphValues &values)
{
    auto wnd_size = rndsys->getWindowSize();

    for (auto v : values) {
        auto graph_found = graphs.find(v.first);

        if (graph_found == graphs.end())
            continue;

        graph_found->second->update(graph_found->first, v.second, 10, wnd_size.Y - 10);
    }
}

void ProfilerGraphSet::draw() const
{
    if (!is_visible)
        return;
    for (auto &graph : graphs)
        graph.second->draw();
}
