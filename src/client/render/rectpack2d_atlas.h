#pragma once

#include "atlas.h"
#include <rectpack2D/finders_interface.h>
#include <unordered_map>

using namespace rectpack2D;

#define CALC_MAX_MIP_LEVEL(side) \
    (u8)std::ceil(std::log2((f32)side))

#define CALC_CLOSEST_POT_SIDE(area) \
    std::pow(2u, (u32)std::ceil(std::log2(std::sqrt((f32)area))))


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
    std::vector<u32> animatedTiles;

    std::vector<rect_xywh> rects;

    u32 maxSize;
    u32 actualSize;

    ResourceCache *cache;
    u32 frameThickness;

    bool mipMaps;
    bool filtering;

    std::vector<rectu> freeSpaces; // used for the manual per-a-tile packing
public:
    Rectpack2DAtlas(ResourceCache *_cache, const std::string &name, u32 num, u32 maxTextureSize, img::Image *img, bool filtered);
    Rectpack2DAtlas(ResourceCache *_cache,const std::string &name, u32 num, u32 maxTextureSize, bool filtered,
        const std::vector<img::Image *> &images, const std::unordered_map<u32, std::pair<u32, u32>> &animatedImages, u32 &start_i);

    void packTiles() override;
    bool packSingleTile(img::Image *img, u32 num);

    void updateAnimatedTiles(f32 time);
private:
    void splitToTwoSubAreas(rectu area, rectu r, std::vector<rectu> &newFreeSpaces);

    void recreateImageWithFrame(img::Image **img);
};
