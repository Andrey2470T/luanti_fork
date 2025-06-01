#include "rectpack2d_atlas.h"

using empty_rects = empty_spaces<false>;

rectu AnimatedAtlasTile::getFrameCoords(u32 frame_num) const
{
    frame_num = std::clamp<u32>((frame_count - 1) - frame_num, 0, frame_count-1);
    
    u32 h = image->getHeight();
    return rectu(
        0,
        (u32)((f32)frame_num / frame_count * h),
        size.X,
        (u32)((f32)((frame_num + 1) / frame_count) * h)
    );
}

void AnimatedAtlasTile::updateFrame(f32 time)
{
    cur_frame = (u32)(time * 1000 / frame_length_ms) % frame_count;
}

Rectpack2DAtlas::Rectpack2DAtlas(const std::string &name, u32 num, u32 maxTextureSize, bool hasMips,
        const std::vector<img::Image *> &images, const std::unordered_map<u32, std::pair<u32, u32> > &animatedImages, u32 &start_i)
    : Atlas(), maxSize(maxTextureSize)
{
    u32 atlasArea = 0;
    u32 maxArea = maxSize * maxSize;

    for (; start_i < images.size(); start_i++) {
        auto tileImg = images.at(start_i);
        v2u size = tileImg->getClipSize();
        u32 tileArea = size.X * size.Y;

        if ((atlasArea + tileArea) > maxArea)
            break;

        AtlasTile *newTile = nullptr;
        auto animationProps = animatedImages.find(start_i);

        // Animated images have the clip region restricting within the top frame
        if (animationProps != animatedImages.end()) {
            newTile = new AnimatedAtlasTile(tileImg, num, animationProps->second.first, animationProps->second.second);
            animatedTiles.push_back(start_i);
        }
        else
            newTile = new AtlasTile(tileImg, num);

        bool added = addTile(newTile);

        if (added)
            atlasArea += tileArea;
    }

    actualSize = std::pow(2u, (u32)std::ceil(std::log2(std::sqrt((f32)atlasArea))));
    packTiles();

    u8 maxMipLevel = hasMips ? (u8)std::ceil(std::log2((f32)actualSize)) : 0;
    createTexture(name, num, actualSize, maxMipLevel);
    drawTiles();
}

void Rectpack2DAtlas::packTiles()
{
    for (auto &tile : tiles)
        rects.emplace_back(0, 0, tile->size.X, tile->size.Y);

    auto report_successful = [](rect_xywh&) {
        return callback_result::CONTINUE_PACKING;
    };

    auto report_unsuccessful = [](rect_xywh&) {
        return callback_result::ABORT_PACKING;
    };

    auto res_size = find_best_packing<empty_rects>(rects,
        make_finder_input(actualSize, 1, report_successful, report_unsuccessful, flipping_option::DISABLED));
    actualSize = std::max(res_size.w, res_size.h);

    for (u32 i = 0; i < rects.size(); i++)
        tiles[i]->pos = v2u(rects.at(i).x, rects.at(i).y);

    rects.clear();
}

void Rectpack2DAtlas::updateAnimatedTiles(f32 time)
{
    if (animatedTiles.empty())
        return;

    bool animationsUpdated = false;
    for (u32 anim_i : animatedTiles) {
        if (anim_i > tiles.size()-1)
            continue;

        auto *tile = dynamic_cast<AnimatedAtlasTile*>(getTile(anim_i));

        if (tile->frame_count == 0)
            continue;

        u32 prevFrame = tile->cur_frame;
        tile->updateFrame(time);

        if (prevFrame == tile->cur_frame)
            continue;

        animationsUpdated = true;
        rectu frame_coords = tile->getFrameCoords(tile->cur_frame);
        tile->image->setClipRegion(
            frame_coords.ULC.X, frame_coords.ULC.Y,
            frame_coords.getWidth(), frame_coords.getHeight());

        markDirty(anim_i);
    }

    if (animationsUpdated)
        drawTiles();
}
