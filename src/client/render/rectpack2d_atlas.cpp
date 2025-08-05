#include "rectpack2d_atlas.h"
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
    cur_frame = (u32)(time * 1000 / frame_length_ms) % frame_count;
}

Rectpack2DAtlas::Rectpack2DAtlas(ResourceCache *_cache, const std::string &name, u32 num, u32 maxTextureSize, img::Image *img, bool filtered,
    std::optional<AtlasTileAnim> anim)
    : Atlas(), maxSize(maxTextureSize), cache(_cache)
{
    mipMaps = filtered && g_settings->getBool("mip_map");
    filtering = filtered && (g_settings->getBool("bilinear_filter") ||
        g_settings->getBool("trilinear_filter") || g_settings->getBool("anisotropic_filter"));

    v2u clipSize = img->getClipSize();
    u8 maxMipLevel = mipMaps ? CALC_MAX_MIP_LEVEL(std::min(clipSize.X, clipSize.Y)) : 0;
    frameThickness = maxMipLevel + 4;

    if (filtering)
        recreateImageWithFrame(&img);
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
    tile->pos = v2u(0, actualSize);

    splitToTwoSubAreas(rectu(tile->pos, actualSize, actualSize), rectu(tile->pos, tile->size), freeSpaces);

    createTexture(name, num, actualSize, maxMipLevel);
}

Rectpack2DAtlas::Rectpack2DAtlas(ResourceCache *_cache, const std::string &name, u32 num, u32 maxTextureSize, bool filtered,
    const std::vector<img::Image *> &images, const std::unordered_map<u32, AtlasTileAnim> &animatedImages, u32 &start_i)
    : Atlas(), maxSize(maxTextureSize), cache(_cache)
{
    mipMaps = filtered && g_settings->getBool("mip_map");
    filtering = filtered && (g_settings->getBool("bilinear_filter") ||
        g_settings->getBool("trilinear_filter") || g_settings->getBool("anisotropic_filter"));

    u8 maxMipLevel = 0;

    if (mipMaps) {
        u32 minImgSize = std::min(images.at(start_i)->getClipSize().X, images.at(start_i)->getClipSize().Y);
        for (u32 i = start_i; i < images.size(); i++) {
            u32 curImgSize = std::min(images.at(i)->getClipSize().X, images.at(i)->getClipSize().Y);
            minImgSize = std::min(minImgSize, curImgSize);
        }

        maxMipLevel = CALC_MAX_MIP_LEVEL(minImgSize);
        frameThickness = maxMipLevel + 4;
    }

    u32 atlasArea = 0;
    u32 maxArea = maxSize * maxSize;

    for (; start_i < images.size(); start_i++) {
        auto tileImg = images.at(start_i);

        if (filtering)
            recreateImageWithFrame(&tileImg);
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

bool Rectpack2DAtlas::packSingleTile(img::Image *img, u32 num, std::optional<AtlasTileAnim> anim)
{
    AtlasTile *newTile;

    if (anim)
        newTile = new AnimatedAtlasTile(img, num, anim.value().first, anim.value().second);
    else
        newTile = new AtlasTile(img, num);

    // At first try to place the tile in one of the free spaces
    bool packed = false;

    rectu tileRect(0, 0, newTile->size.X, newTile->size.Y);
    std::vector<rectu> newFreeSpaces;
    for (auto &space : freeSpaces) {
        tileRect.ULC = space.ULC;
        tileRect.LRC += space.ULC;
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
        u32 newAtlasSize = CALC_CLOSEST_POT_SIDE(getTextureSize() + newTile->size.Y);

        if (newAtlasSize > maxSize)
            return false;

        getTexture()->resize(newAtlasSize, newAtlasSize, g_imgmodifier);

        newTile->pos = v2u(0, newAtlasSize);
        rectu upper_space(newTile->size.X, newAtlasSize, newAtlasSize, newAtlasSize-newTile->size.Y);
        rectu right_space(actualSize, newAtlasSize-newTile->size.Y, newAtlasSize, 0);
        freeSpaces.push_back(upper_space);
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

void Rectpack2DAtlas::splitToTwoSubAreas(rectu area, rectu r, std::vector<rectu> &newFreeSpaces)
{
    if (area.getSize() == r.getSize())
        return;

    u32 area_w = area.getWidth();
    u32 area_h = area.getHeight();
    u32 r_w = r.getWidth();
    u32 r_h = r.getHeight();

    if (area_w == r_w) {
        newFreeSpaces.emplace_back(area.ULC.X, area.ULC.Y-r_h, area.LRC.X, area.LRC.Y);
        return;
    }

    if (area_h == r_h) {
        newFreeSpaces.emplace_back(area.ULC.X+r_w, area.ULC.Y, area.LRC.X, area.LRC.Y);
        return;
    }

    newFreeSpaces.emplace_back(area.ULC.X, area.ULC.Y-r_h, area.LRC.X, area.LRC.Y);
    newFreeSpaces.emplace_back(area.ULC.X+r_w, area.ULC.Y, area.LRC.X, area.ULC.Y-r_h);
}

void Rectpack2DAtlas::recreateImageWithFrame(img::Image **img)
{
    // Download the pixel data of the tile from GPU into IImage
    v2u old_size = (*img)->getSize();
    img::PixelFormat format = (*img)->getFormat();

    v2u new_size(
        old_size.X + 2 * frameThickness,
        old_size.Y + 2 * frameThickness
    );

    // Create the increased image from the previous IImage with a pixel frame of the certain thickness
    img::Image *ext_img = new img::Image(format, new_size.X, new_size.Y);

    v2u img_offset(frameThickness, frameThickness);
    rectu ext_img_rect(
        img_offset.X, img_offset.Y,
        img_offset.X + old_size.X, img_offset.Y + old_size.Y);
    g_imgmodifier->copyTo(*img, ext_img, nullptr, &ext_img_rect);

    auto calc_frame_strip = [this] (u32 imgSide, u32 side, u8 dir, u32 shift) -> rectu
    {
        recti strip_r;
        v2i shift_v;

        switch (dir) {
        case 0: {// to right
            strip_r = recti(0, side, 1, 0);
            shift_v = v2i(1, frameThickness);
            break;
        }
        case 1: // to bottom
            strip_r = recti(0, side, side, side-1);
            shift_v = v2i(frameThickness, -1);
            break;
        case 2: // to left
            strip_r = recti(side-1, side, side, 0);
            shift_v = v2i(-1, imgSide - frameThickness);
            break;
        case 3: // to top
            strip_r = recti(0, 1, side, 0);
            shift_v = v2i(frameThickness, 1);
            break;
        }

        strip_r += shift_v * shift;

        return rectu(strip_r.ULC.X, strip_r.ULC.Y, strip_r.LRC.X, strip_r.LRC.Y);
    };

    auto drawFrameQuarter = [this, calc_frame_strip, img, ext_img] (u32 imgSide, u32 side, u8 dir)
    {
        rectu srcrect;

        switch (dir) {
        case 0:
            srcrect = rectu(0, side, 1, 0);
            break;
        case 1:
            srcrect = rectu(0, side, side, side-1);
            break;
        case 2:
            srcrect = rectu(side-1, side, side, 0);
            break;
        case 3:
            srcrect = rectu(0, 1, side, 0);
            break;
        }

        for (u32 i = 0; i < frameThickness; i++) {
            rectu destrect = calc_frame_strip(imgSide, side, dir, i);
            g_imgmodifier->copyTo(*img, ext_img, &srcrect, &destrect);
        }
    };

    drawFrameQuarter(new_size.Y, old_size.Y, 0);
    drawFrameQuarter(new_size.X, old_size.X, 1);
    drawFrameQuarter(new_size.Y, old_size.Y, 2);
    drawFrameQuarter(new_size.X, old_size.X, 3);

    cache->cacheResource<img::Image>(ResourceType::IMAGE, ext_img);
    *img = ext_img;
}
