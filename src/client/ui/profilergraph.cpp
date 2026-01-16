// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
// Copyright (C) 2018 nerzhul, Loic Blot <loic.blot@unix-experience.fr>

#include "porting.h"
#include "profilergraph.h"
#include "client/render/rendersystem.h"
#include "text_sprite.h"
#include "util/string.h"

ProfilerGraph::ProfilerGraph(SpriteDrawBatch *_drawBatch, u8 _number, img::color8 _color)
    : drawBatch(_drawBatch), color(_color), number(_number)
{
    graph = drawBatch->addRectsSprite(log_max_size);
    text = drawBatch->addTextSprite(L"", 0, std::nullopt, color, nullptr, true);
}

void ProfilerGraph::update(const std::string &id, f32 new_value, s32 x_left, s32 y_bottom)
{
    values.emplace_back(new_value);

    while (values.size() > log_max_size)
        values.erase(values.begin());

    f32 min_value = values.front();
    f32 max_value = values.front();
    for (f32 &v : values) {
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
    
    sts::list<f32> fullValues = values;
    fullValues.resize(log_max_size);

    for (u32 k = 0; k < fullValues.size(); k++) {
    	f32 &v = fullValues.at(k);
        float scaledvalue = 1.0;

        if (show_max != show_min)
            scaledvalue = (v - show_min) / (show_max - show_min);

        img::color8 actualColor = color;

        if (v == 0)
            actualColor.A(0);

        v2f start_pos, end_pos;
        if (relativegraph) {
            s32 ivalue1 = lastscaledvalue * graph1h;
            s32 ivalue2 = scaledvalue * graph1h;

            start_pos = v2f(x - 1, graph1y - ivalue1);
            end_pos = v2f(x, graph1y - ivalue2);

            lastscaledvalue = scaledvalue;
        } else {
            s32 ivalue = scaledvalue * graph1h;

            start_pos = v2f(x, graph1y);
            end_pos = v2f(x, graph1y - ivalue);
        }

        v2f normal = (end_pos - start_pos).normalize();
        normal.rotateBy(90);

        rectf line_r(start_pos, end_pos+normal);

        graph->updateRect(k, {line_r, actualColor});

        x++;
    }
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
    text->setBoundRect(rectf(ulc_x, ulc_y, lrc_x, lrc_y));
}

void ProfilerGraphSet::put(const Profiler::GraphValues &values)
{
    if (!is_visible)
        return;

    auto wnd_size = rndsys->getWindowSize();

    for (auto v : values)
        graphs.at(v.first)->update(v.first, v.second, 10, wnd_size.Y - 10);
}

void ProfilerGraphSet::draw() const
{
    if (!is_visible)
        return;

    drawBatch->rebuild();
    drawBatch->draw();
}
