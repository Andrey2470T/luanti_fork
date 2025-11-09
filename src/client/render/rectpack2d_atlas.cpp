#include "rectpack2d_atlas.h"
#include "Image/ImageLoader.h"
#include "renderer.h"
#include "settings.h"
#include "client/media/resource.h"

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
    cur_frame = (u32)(time * 1000 / frame_length_ms + frame_offset * frame_length_ms / frame_count) % frame_count;
}

Rectpack2DAtlas::Rectpack2DAtlas(ResourceCache *_cache, const std::string &name, u32 num, u32 maxTextureSize, img::Image *img, bool filtered,
    std::optional<AtlasTileAnim> anim)
    : Atlas(), maxSize(maxTextureSize), cache(_cache)
{
    mipMaps = filtered && g_settings->getBool("mip_map");
    filtering = filtered && (g_settings->getBool("bilinear_filter") ||
        g_settings->getBool("trilinear_filter") || g_settings->getBool("anisotropic_filter"));

    AtlasTile *tile = nullptr;

    if (anim)
        tile = new AnimatedAtlasTile(img, num, anim.value().first, anim.value().second);
    else
        tile = new AtlasTile(img, num);

    bool added = addTile(tile);

    if (!added)
        return;

    if (anim)
        animatedTiles.push_back(tiles.size()-1);

    actualSize = std::max(tile->size.X, tile->size.Y);
    tile->pos = v2u(0);

    splitToTwoSubAreas(rectu(tile->pos, actualSize, actualSize), rectu(tile->pos, tile->size), freeSpaces);

    std::string atlasName = name + "_" + std::to_string(num);
    createTexture(atlasName, actualSize);
    drawTiles();
}

Rectpack2DAtlas::Rectpack2DAtlas(ResourceCache *_cache, const std::string &name, u32 num, u32 maxTextureSize, bool filtered,
    const std::vector<img::Image *> &images, const std::unordered_map<u32, AtlasTileAnim> &animatedImages, u32 &start_i)
    : Atlas(), maxSize(maxTextureSize), cache(_cache)
{
    mipMaps = filtered && g_settings->getBool("mip_map");
    filtering = filtered && (g_settings->getBool("bilinear_filter") ||
        g_settings->getBool("trilinear_filter") || g_settings->getBool("anisotropic_filter"));

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
        if (animationProps != animatedImages.end())
            newTile = new AnimatedAtlasTile(tileImg, num, animationProps->second.first, animationProps->second.second);
        else
            newTile = new AtlasTile(tileImg, num);

        bool added = addTile(newTile);

        if (added) {
            atlasArea += tileArea;

            if (animationProps != animatedImages.end())
                animatedTiles.push_back(start_i);
        }
    }

    actualSize = CALC_CLOSEST_POT_SIDE(atlasArea);
    packTiles();

    std::string atlasName = name + "_" + std::to_string(num);
    createTexture(atlasName, actualSize);
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

bool Rectpack2DAtlas::packSingleTile(img::Image *img, u32 num, std::optional<AtlasTileAnim> anim)
{
    AtlasTile *newTile;

    if (anim)
        newTile = new AnimatedAtlasTile(img, num, anim.value().first, anim.value().second);
    else
        newTile = new AtlasTile(img, num);

    // At first try to place the tile in one of the free spaces
    bool packed = false;

    std::vector<rectu> newFreeSpaces;
    for (auto &space : freeSpaces) {
        rectu tileRect(0, 0, newTile->size.X, newTile->size.Y);
        tileRect += space.ULC;

        if (canFit(space, tileRect)) {
            newTile->pos = space.ULC;
            splitToTwoSubAreas(space, tileRect, newFreeSpaces);
            packed = true;
        }
        else
            newFreeSpaces.emplace_back(space);
    }

    freeSpaces = newFreeSpaces;

    // If couldn't pack, try to increase the atlas size
    if (!packed) {
        u32 oldAtlasSize = getTextureSize();
        u32 newAtlasSize = CALC_CLOSEST_POT_SIDE((oldAtlasSize + newTile->size.Y)*(oldAtlasSize + newTile->size.Y));

        if (newAtlasSize > maxSize) {
            delete newTile;
            return false;
        }

        getTexture()->resize(newAtlasSize, newAtlasSize, g_imgmodifier);

        newTile->pos = v2u(0, oldAtlasSize);
        rectu upper_right_space(newTile->size.X, oldAtlasSize, newAtlasSize, newAtlasSize);
        rectu right_space(oldAtlasSize, 0, newAtlasSize, oldAtlasSize);
        rectu upper_left_space(0, oldAtlasSize+newTile->size.Y, newTile->size.X, newAtlasSize);
        freeSpaces.push_back(upper_right_space);
        freeSpaces.push_back(upper_left_space);

        freeSpaces.push_back(right_space);

        actualSize = newAtlasSize;
    }

    bool added = addTile(newTile);
    if (!added) {
        delete newTile;
        return false;
    }
    if (anim)
        animatedTiles.push_back(tiles.size()-1);

    drawTiles();

    return true;
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

std::string Rectpack2DAtlas::getName(u32 size, u32 num) const
{
    return texture->getName();
}

void Rectpack2DAtlas::splitToTwoSubAreas(rectu area, rectu r, std::vector<rectu> &newFreeSpaces)
{
    if (area.getSize() == r.getSize())
        return;

    u32 area_w = area.getWidth();
    u32 area_h = area.getHeight();
    u32 r_w = r.getWidth();
    u32 r_h = r.getHeight();

    if (area_w == r_w) {
        newFreeSpaces.emplace_back(area.ULC.X, area.ULC.Y+r_h, area.LRC.X, area.LRC.Y);
        return;
    }

    if (area_h == r_h) {
        newFreeSpaces.emplace_back(area.ULC.X+r_w, area.ULC.Y, area.LRC.X, area.LRC.Y);
        return;
    }

    newFreeSpaces.emplace_back(area.ULC.X, area.ULC.Y+r_h, area.LRC.X, area.LRC.Y);
    newFreeSpaces.emplace_back(area.ULC.X+r_w, area.ULC.Y, area.LRC.X, area.ULC.Y+r_h);
}
