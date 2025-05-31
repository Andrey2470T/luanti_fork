#pragma once

#include "atlas.h"
#include <rectpack2D/finders_interface.h>
#include <unordered_map>

using namespace rectpack2D;

struct AnimatedAtlasTile : public AtlasTile
{
    u32 frame_length_ms;
	u32 frame_count = 1;
	
	u32 cur_frame = 0;

    AnimatedAtlasTile(img::Image *img, u32 num, u32 length, u32 count)
        : AtlasTile(img, num), frame_length_ms(length), frame_count(count)
    {}
    rectu getFrameCoords(u32 frame_num) const;
    void updateFrame(f32 time); // in sec
};

class Rectpack2DAtlas : public Atlas
{
    // Saves only unique tiles (determined by the hash)
    std::unordered_map<size_t, u32> hash_to_index;

    std::vector<std::unique_ptr<AtlasTile>> tiles;
    std::vector<u32> animatedTiles;

    std::vector<rect_xywh> rects;

    u32 maxSize;
    u32 actualSize;
public:
    Rectpack2DAtlas(const std::string &name, u32 num, u32 maxTextureSize, bool hasMips,
        const std::vector<img::Image *> &images, const std::unordered_map<u32, std::pair<u32, u32>> &animatedImages, u32 &start_i);

    bool addTile(const AtlasTile *tile);
    AtlasTile *getTile(u32 i) const;

    void packTiles() override;

    void drawTiles() override;

    void updateAnimatedTiles(f32 time);
};
