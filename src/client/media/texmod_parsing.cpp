#include "texture_modifiers.h"
#include "util/strfnd.h"
#include "util/string.h"
#include "util/base64.h"
#include "log.h"
#include "Image/ImageFilters.h"
#include "Image/ImageLoader.h"
#include "Image/Converting.h"

#define COMPLAIN_INVALID(description, mod) \
do { \
        errorstream << "TextureGenerator::generatePart(): invalid " << (description) \
        << " for part_of_name=\"" << mod \
        << "\", cancelling." << std::endl; \
        return false; \
} while(0)

#define CHECK_DIM(w, h, mod) \
    do { \
        if ((w) <= 0 || (h) <= 0 || (w) >= 0xffff || (h) >= 0xffff) { \
            COMPLAIN_INVALID("width or height", mod); \
    } \
} while(0)

/*
    [crack:N:P
    [cracko:N:P
    Adds a cracking texture
    N = animation frame count, P = crack progression
*/
bool TexModParser::parseCrack(TextureGenerator *texgen, img::Image *dest, const std::string &mod)
{
    // Crack image number and overlay option
    // Format: crack[o][:<tiles>]:<frame_count>:<frame>
    //bool use_overlay = (mod[6] == 'o');
    Strfnd sf(mod);
    sf.next(":");
    s32 frame_count = stoi(sf.next(":"));
    s32 progression = stoi(sf.next(":"));
    s32 tiles = 1;
    // Check whether there is the <tiles> argument, that is,
    // whether there are 3 arguments. If so, shift values
    // as the first and not the last argument is optional.
    auto s = sf.next(":");
    if (!s.empty()) {
        tiles = frame_count;
        frame_count = progression;
        progression = stoi(s);
    }

    if (progression >= 0) {
        /*
            Load crack image.

            It is an image with a number of cracking stages
            horizontally tiled.
        */
        img::Image *img_crack = texgen->resCache->getOrLoad<img::Image>(
            ResourceType::IMAGE, "crack_anylength.png")->data.get();

        if (!img_crack)
            return false;

        // Dimension of destination image
        auto dest_size = dest->getSize();
        // Limit frame_count
        frame_count = std::clamp<s32>(frame_count, 1, dest_size.Y);
        // Dimension of the scaled crack stage,
        // which is the same as the dimension of a single destination frame
        v2u frame_size(dest_size.X, dest_size.Y / frame_count);
        img::Image *crack_scaled = texgen->createCrack(img_crack, progression,
            frame_size, tiles);
        if (!crack_scaled)
            return false;

        for (s32 i = 0; i < frame_count; ++i)
            texgen->blitImages(crack_scaled, dest, v2u(0, frame_size.Y * i), &frame_size);

        delete crack_scaled;
        /*draw_crack(img_crack, baseimg,
                use_overlay, frame_count,
                progression, driver, tiles);
        img_crack->drop();*/
    }

    return true;
}

/*
    [combine:WxH:X,Y=filename:X,Y=filename2
    Creates a bigger texture from any amount of smaller ones
*/
bool TexModParser::parseCombine(TextureGenerator *texgen, img::Image *dest, const std::string &mod)
{
    Strfnd sf(mod);
    sf.next(":");
    u32 w0 = stoi(sf.next("x"));
    u32 h0 = stoi(sf.next(":"));
    if (!dest) {
        CHECK_DIM(w0, h0, mod);
        dest = new img::Image(img::PF_RGBA8, w0, h0);
    }

    while (!sf.at_end()) {
        v2u pos_base;
        pos_base.X = stoi(sf.next(","));
        pos_base.Y = stoi(sf.next("="));
        std::string filename = unescape_string(sf.next_esc(":", '\\'));

        auto basedim = dest->getSize();
        if (pos_base.X > basedim.X || pos_base.Y > basedim.Y) {
            warningstream << "TexModParser::parseCombine(): Skipping \""
                          << filename << "\" as it's out-of-bounds " << pos_base
                          << " for [combine" << std::endl;
            continue;
        }
        infostream << "Adding \"" << filename<< "\" to combined "
                   << pos_base << std::endl;

        img::Image *img = texgen->generate(filename);
        if (!img) {
            errorstream << "TexModParser::parseCombine(): Failed to load image \""
                        << filename << "\" for [combine" << std::endl;
            continue;
        }
        const auto dim = img->getSize();
        if (pos_base.X + dim.X <= 0 || pos_base.Y + dim.Y <= 0) {
            warningstream << "TexModParser::parseCombine(): Skipping \""
                          << filename << "\" as it's out-of-bounds " << pos_base
                          << " for [combine" << std::endl;
            delete img;
            continue;
        }

        texgen->blitImages(img, dest, pos_base, &dim);
        delete img;
    }

    return true;
}

/*
    [fill:WxH:color
    [fill:WxH:X,Y:color
    Creates a texture of the given size and color, optionally with an <x>,<y>
    position. An alpha value may be specified in the `Colorstring`.
*/
bool TexModParser::parseFill(TextureGenerator *texgen, img::Image *dest, const std::string &mod)
{
    u32 x = 0;
    u32 y = 0;

    Strfnd sf(mod);
    sf.next(":");
    u32 width  = stoi(sf.next("x"));
    u32 height = stoi(sf.next(":"));
    std::string color_or_x = sf.next(",");

    img::color8 color;
    if (!parseColorString(color_or_x, color, true)) {
        x = stoi(color_or_x);
        y = stoi(sf.next(":"));
        std::string color_str = sf.next(":");

        if (!parseColorString(color_str, color, false))
            return false;
    }

    CHECK_DIM(width, height, mod);
    if (dest) {
        auto basedim = dest->getSize();
        if (x >= basedim.X || y >= basedim.Y)
            COMPLAIN_INVALID("X or Y offset", mod);
    }

    img::Image *img = new img::Image(img::PF_RGBA8, width, height, color);

    if (!dest) {
        dest = img;
    } else {
        v2u size(width, height);
        texgen->blitImages(img, dest, v2u(x, y), &size);
        delete img;
    }

    return true;
}

template <typename F>
void applyToImage(TextureGenerator *texgen, img::Image *img, const F &func)
{
    auto *mdf = texgen->getImageModifier();
    auto size = img->getSize();

    for (u32 y=0; y < size.Y; y++)
        for (u32 x=0; x < size.X; x++) {
            img::color8 c = mdf->getPixelColor(img, x,y);

            if (!func(c))
                continue;
            mdf->setPixelColor(img, x, y, c);
        }
}

template <typename F>
void applyToImageFromSrc(TextureGenerator *texgen, img::Image *srcImg, img::Image *destImg, const F &func)
{
    auto *mdf = texgen->getImageModifier();
    auto size = srcImg->getSize();

    for (u32 y=0; y < size.Y; y++)
        for (u32 x=0; x < size.X; x++) {
            img::color8 src_c = mdf->getPixelColor(srcImg, x,y);
            img::color8 dest_c = mdf->getPixelColor(destImg, x,y);

            if (!func(src_c, dest_c))
                continue;
            mdf->setPixelColor(destImg, x, y, dest_c);
        }
}

/*
    [brighten
*/
bool TexModParser::parseBrighten(TextureGenerator *texgen, img::Image *dest, const std::string &mod)
{
    auto brighten = [] (img::color8 &input_c)
    {
        input_c.R(0.5 * 255 + 0.5 * (f32)input_c.R());
        input_c.G(0.5 * 255 + 0.5 * (f32)input_c.G());
        input_c.B(0.5 * 255 + 0.5 * (f32)input_c.B());

        return true;
    };

    applyToImage(texgen, dest, brighten);

    return true;
}

/*
    [noalpha
    Make image completely opaque.
    Used for the leaves texture when in old leaves mode, so
    that the transparent parts don't look completely black
    when simple alpha channel is used for rendering.
*/
bool TexModParser::parseNoAlpha(TextureGenerator *texgen, img::Image *dest, const std::string &mod)
{
    auto noAlpha = [] (img::color8 &input_c)
    {
        input_c.A(255);

        return true;
    };

    applyToImage(texgen, dest, noAlpha);

    return true;
}

/*
    [makealpha:R,G,B
    Convert one color to transparent.
*/
bool TexModParser::parseMakeAlpha(TextureGenerator *texgen, img::Image *dest, const std::string &mod)
{
    Strfnd sf(mod.substr(11));
    u32 r1 = stoi(sf.next(","));
    u32 g1 = stoi(sf.next(","));
    u32 b1 = stoi(sf.next(""));

    auto makeAlpha = [&] (img::color8 &input_c)
    {
        if (!(input_c.R() == r1 && input_c.G() == g1 && input_c.B() == b1))
            return false;
        input_c.A(0);

        return true;
    };

    applyToImage(texgen, dest, makeAlpha);

    return true;
}

/*
    [transformN
    Rotates and/or flips the image.

    N can be a number (between 0 and 7) or a transform name.
    Rotations are counter-clockwise.
    0  I      identity
    1  R90    rotate by 90 degrees
    2  R180   rotate by 180 degrees
    3  R270   rotate by 270 degrees
    4  FX     flip X
    5  FXR90  flip X then rotate by 90 degrees
    6  FY     flip Y
    7  FYR90  flip Y then rotate by 90 degrees

    Note: Transform names can be concatenated to produce
    their product (applies the first then the second).
    The resulting transform will be equivalent to one of the
    eight existing ones, though (see: dihedral group).
*/
bool TexModParser::parseTransform(TextureGenerator *texgen, img::Image *dest, const std::string &mod)
{
    u32 transform = parseImageTransform(mod.substr(10));

    if (transform > 6) {
        infostream << "TexModParser::parseTransform(): unknown transform index\n";
        return false;
    }
    auto size = dest->getSize();

    if (transform % 2 != 0)
        std::swap(size.X, size.Y);

    img::Image *image = new img::Image(dest->getFormat(), size.X, size.Y);

    /*
        Compute the transformation from source coordinates (sx,sy)
        to destination coordinates (dx,dy).
    */
    int sxn = 0;
    int syn = 2;
    if (transform == 0)         // identity
        sxn = 0, syn = 2;  //   sx = dx, sy = dy
    else if (transform == 1)    // rotate by 90 degrees ccw
        sxn = 3, syn = 0;  //   sx = (H-1) - dy, sy = dx
    else if (transform == 2)    // rotate by 180 degrees
        sxn = 1, syn = 3;  //   sx = (W-1) - dx, sy = (H-1) - dy
    else if (transform == 3)    // rotate by 270 degrees ccw
        sxn = 2, syn = 1;  //   sx = dy, sy = (W-1) - dx
    else if (transform == 4)    // flip x
        sxn = 1, syn = 2;  //   sx = (W-1) - dx, sy = dy
    else if (transform == 5)    // flip x then rotate by 90 degrees ccw
        sxn = 2, syn = 0;  //   sx = dy, sy = dx
    else if (transform == 6)    // flip y
        sxn = 0, syn = 3;  //   sx = dx, sy = (H-1) - dy
    else if (transform == 7)    // flip y then rotate by 90 degrees ccw
        sxn = 3, syn = 1;  //   sx = (H-1) - dy, sy = (W-1) - dx

    for (u32 dy=0; dy<size.Y; dy++)
        for (u32 dx=0; dx<size.X; dx++)
        {
            u32 entries[4] = {dx, size.X-1-dx, dy, size.Y-1-dy};
            u32 sx = entries[sxn];
            u32 sy = entries[syn];
            auto c = texgen->imgMdf->getPixelColor(dest, sx, sy);
            texgen->imgMdf->setPixelColor(image, dx, dy, c);
        }

    delete dest;
    dest = image;

    return true;
}

/*
    [inventorycube{topimage{leftimage{rightimage
    In every subimage, replace ^ with &.
    Create an "inventory cube".
    NOTE: This should be used only on its own.
    Example (a grass block (not actually used in game):
    "[inventorycube{grass.png{mud.png&grass_side.png{mud.png&grass_side.png"
*/
bool TexModParser::parseInventoryCube(TextureGenerator *texgen, img::Image *dest, const std::string &mod)
{
    if (dest) {
        errorstream<<"TexModParser::parseInventoryCube(): baseimg != NULL "
                    <<"for part_of_name=\""<< mod
                    <<"\", cancelling."<<std::endl;
        return false;
    }

    std::string part_s(mod);
    str_replace(part_s, '&', '^');

    Strfnd sf(part_s);
    sf.next("{");
    std::string imagename_top = sf.next("{");
    std::string imagename_left = sf.next("{");
    std::string imagename_right = sf.next("{");

    // Generate images for the faces of the cube
    img::Image *img_top = texgen->generate(imagename_top);
    img::Image *img_left = texgen->generate(imagename_left);
    img::Image *img_right = texgen->generate(imagename_right);

    if (!img_top || !img_left || !img_right) {
        errorstream << "TexModParser::parseInventoryCube(): Failed to create textures"
                    << " for inventorycube \"" << mod << "\""
                    << std::endl;
        return false;
    }

    dest = texgen->createInventoryCubeImage(img_top, img_left, img_right);

    // Face images are not needed anymore
    delete img_top;
    delete img_left;
    delete img_right;

    return true;
}

/*
    [lowpart:percent:filename
    Adds the lower part of a texture
*/
bool TexModParser::parseLowPart(TextureGenerator *texgen, img::Image *dest, const std::string &mod)
{
    Strfnd sf(mod);
    sf.next(":");
    u32 percent = stoi(sf.next(":"), 0, 100);
    std::string filename = unescape_string(sf.next_esc(":", '\\'));

    img::Image *img = texgen->generate(filename);
    if (!img) {
        errorstream << "TexModParser::parseLowPart(): Failed to load image"
                    << filename << " for [lowpart" << std::endl;
        return false;
    }

    auto size = img->getSize();
    if (!dest)
        dest = new img::Image(img::PF_RGBA8, size.X, size.Y);

    v2u srcPos(0, size.Y * (100-percent) / 100);
    v2u srcSize = size;
    srcSize.Y = srcSize.Y * percent / 100 + 1;
    rectu srcRect(srcPos, srcSize);
    rectu destRect(v2u(0, 0), size);
    texgen->imgMdf->setBlendMode(img::BM_NORMAL);
    texgen->imgMdf->copyTo(img, dest, &srcRect, &destRect);
    delete img;

    return true;
}

/*
    [verticalframe:N:I
    Crops a frame of a vertical animation.
    N = frame count, I = frame index
*/
bool TexModParser::parseVerticalFrame(TextureGenerator *texgen, img::Image *dest, const std::string &mod)
{
    Strfnd sf(mod);
    sf.next(":");
    u32 frame_count = stoi(sf.next(":"));
    u32 frame_index = stoi(sf.next(":"));

    if (frame_count == 0){
        errorstream << "TexModParser::parseVerticalFrame(): invalid frame_count "
                    << "for part_of_name=\"" << mod
                    << "\", using frame_count = 1 instead." << std::endl;
        frame_count = 1;
    }
    if (frame_index >= frame_count)
        frame_index = frame_count - 1;

    v2u frame_size = dest->getSize();
    frame_size.Y /= frame_count;

    img::Image *img = new img::Image(img::PF_RGBA8, frame_size.X, frame_size.Y);

    rectu srcRect(v2u(0, frame_index * frame_size.Y), frame_size);
    texgen->imgMdf->copyTo(dest, img, &srcRect);
    // Replace baseimg
    delete dest;
    dest = img;

    return true;
}

/*
    [mask:filename
    Applies a mask to an image
*/
bool TexModParser::parseMask(TextureGenerator *texgen, img::Image *dest, const std::string &mod)
{
    Strfnd sf(mod);
    sf.next(":");
    std::string filename = unescape_string(sf.next_esc(":", '\\'));

    img::Image *img = texgen->generate(filename);
    if (!img) {
        errorstream << "TexModParser::parseMask(): Failed to load image \""
                    << filename << "\" for [mask" << std::endl;
        return false;
    }

    texgen->upscaleToLargest(dest, img);

    auto mask = [] (img::color8 &src_c, img::color8 &dest_c)
    {
        dest_c.R(dest_c.R() & src_c.R());
        dest_c.G(dest_c.G() & src_c.G());
        dest_c.B(dest_c.B() & src_c.B());
        dest_c.A(dest_c.A() & src_c.A());

        return true;
    };

    applyToImageFromSrc(texgen, img, dest, mask);

    delete img;

    return true;
}

/*
    [multiply:color
    or
    [screen:color
        Multiply and Screen blend modes are basic blend modes for darkening and lightening
        images, respectively.
        A Multiply blend multiplies a given color to every pixel of an image.
        A Screen blend has the opposite effect to a Multiply blend.
        color = color as ColorString
*/
bool TexModParser::parseMultiply(TextureGenerator *texgen, img::Image *dest, const std::string &mod)
{
    Strfnd sf(mod);
    sf.next(":");
    std::string color_str = sf.next(":");

    img::color8 color;

    if (!parseColorString(color_str, color, false))
        return false;

    texgen->imgMdf->setBlendMode(img::BM_MULTIPLY);
    texgen->imgMdf->fill(dest, color);

    return true;
}

bool TexModParser::parseScreen(TextureGenerator *texgen, img::Image *dest, const std::string &mod)
{
    Strfnd sf(mod);
    sf.next(":");
    std::string color_str = sf.next(":");

    img::color8 color;

    if (!parseColorString(color_str, color, false))
        return false;

    texgen->imgMdf->setBlendMode(img::BM_SCREEN);
    texgen->imgMdf->fill(dest, color);

    return true;
}

/*
    [colorize:color:ratio
    Overlays image with given color
    color = color as ColorString
    ratio = optional string "alpha", or a weighting between 0 and 255
*/
bool TexModParser::parseColorize(TextureGenerator *texgen, img::Image *dest, const std::string &mod)
{
    Strfnd sf(mod);
    sf.next(":");
    std::string color_str = sf.next(":");
    std::string ratio_str = sf.next(":");

    img::color8 color;
    int ratio = -1;
    bool keep_alpha = false;

    if (!parseColorString(color_str, color, false))
        return false;

    if (is_number(ratio_str))
        ratio = mystoi(ratio_str, 0, 255);
    else if (ratio_str == "alpha")
        keep_alpha = true;

    u8 alpha = color.A();
    auto size = dest->getSize();
    if ((ratio == -1 && alpha == 255) || ratio == 255) { // full replacement of color
        for (u32 y = 0; y < size.Y; y++)
            for (u32 x = 0; x < size.X; x++) {
                if (keep_alpha) { // replace the color with alpha = dest alpha * color alpha
                    u8 dst_alpha = texgen->imgMdf->getPixelColor(dest, x, y).A();
                    if (dst_alpha > 0) {
                        color.A(dst_alpha * alpha / 255);
                        texgen->imgMdf->setPixelColor(dest, x, y, color);
                    }
                }
                else { // replace the color including the alpha
                    if (texgen->imgMdf->getPixelColor(dest, x, y).A() > 0)
                        texgen->imgMdf->setPixelColor(dest, x, y, color);
                }
            }
    } else {  // interpolate between the color and destination
        f32 interp = (ratio == -1 ? color.A() / 255.0f : ratio / 255.0f);
        for (u32 y = 0; y < size.Y; y++)
            for (u32 x = 0; x < size.X; x++) {
                auto dst_c = texgen->imgMdf->getPixelColor(dest, x, y);
                if (dst_c.A() > 0) {
                    dst_c = color.linInterp(dst_c, interp);
                    texgen->imgMdf->setPixelColor(dest, x, y, dst_c);
                }
            }
    }

    return true;
}

/*
    [applyfiltersformesh
    Internal modifier
*/
bool TexModParser::parseApplyFiltersForMesh(TextureGenerator *texgen, img::Image *dest, const std::string &mod)
{
    if (texgen->meshFilterNeeded)
        img::imageCleanTransparent(dest, 0, texgen->imgMdf);

    //const bool filter = m_setting_trilinear_filter || m_setting_bilinear_filter;
    const s32 scaleto = texgen->meshFilterNeeded ? texgen->minTextureSize : 1;
    if (scaleto > 1) {
        auto size = dest->getSize();

        /* Calculate scaling needed to make the shortest texture dimension
                 * equal to the target minimum.  If e.g. this is a vertical frames
                 * animation, the short dimension will be the real size.
                 */
        const s32 scale = std::max(scaleto / size.X, scaleto / size.Y);

        // Never downscale; only scale up by 2x or more.
        if (scale > 1) {
            u32 w = scale * size.X;
            u32 h = scale * size.Y;
            v2u newsize(w, h);
            img::Image *newimg = new img::Image(dest->getFormat(), newsize.X, newsize.Y);
            texgen->imgMdf->copyTo(dest, newimg, nullptr, nullptr, true);
            delete dest;
            dest = newimg;
        }
    }

    return true;
}

/*
    [resize:WxH
    Resizes the base image to the given dimensions
*/
bool TexModParser::parseResize(TextureGenerator *texgen, img::Image *dest, const std::string &mod)
{
    Strfnd sf(mod);
    sf.next(":");
    u32 width = stoi(sf.next("x"));
    u32 height = stoi(sf.next(""));
    CHECK_DIM(width, height, mod);

    texgen->imgMdf->resize(dest, rectu(v2u(), width, height));

    return true;
}

/*
    [opacity:R
    Makes the base image transparent according to the given ratio.
    R must be between 0 and 255.
    0 means totally transparent.
*/
bool TexModParser::parseOpacity(TextureGenerator *texgen, img::Image *dest, const std::string &mod)
{
    Strfnd sf(mod);
    sf.next(":");

    u32 ratio = mystoi(sf.next(""), 0, 255);

    auto opacity = [&ratio] (img::color8 &c)
    {
        c.A(std::floor((c.A() * ratio) / 255 + 0.5));

        return true;
    };

    applyToImage(texgen, dest, opacity);

    return true;
}

/*
    [invert:mode
    Inverts the given channels of the base image.
    Mode may contain the characters "r", "g", "b", "a".
    Only the channels that are mentioned in the mode string
    will be inverted.
*/
bool TexModParser::parseInvert(TextureGenerator *texgen, img::Image *dest, const std::string &mod)
{
    Strfnd sf(mod);
    sf.next(":");

    std::string mode = sf.next("");
    u32 mask = 0;
    if (mode.find('a') != std::string::npos)
        mask |= 0xff000000UL;
    if (mode.find('r') != std::string::npos)
        mask |= 0x00ff0000UL;
    if (mode.find('g') != std::string::npos)
        mask |= 0x0000ff00UL;
    if (mode.find('b') != std::string::npos)
        mask |= 0x000000ffUL;

    auto invert = [&mask] (img::color8 &c)
    {
        c.R(c.R() ^ mask);
        c.G(c.G() ^ mask);
        c.B(c.B() ^ mask);
        c.A(c.A() ^ mask);

        return true;
    };

    applyToImage(texgen, dest, invert);

    return true;
}

/*
    [sheet:WxH:X,Y
    Retrieves a tile at position X,Y (in tiles)
    from the base image it assumes to be a
    tilesheet with dimensions W,H (in tiles).
*/
bool TexModParser::parseSheet(TextureGenerator *texgen, img::Image *dest, const std::string &mod)
{
    Strfnd sf(mod);
    sf.next(":");
    u32 w0 = stoi(sf.next("x"));
    u32 h0 = stoi(sf.next(":"));
    u32 x0 = stoi(sf.next(","));
    u32 y0 = stoi(sf.next(":"));

    CHECK_DIM(w0, h0, mod);
    if (x0 >= w0 || y0 >= h0)
        COMPLAIN_INVALID("tile position (X,Y)", mod);

    auto size = dest->getSize();
    v2u tile_dim(size / v2u(w0, h0));
    if (tile_dim.X == 0)
        tile_dim.X = 1;
    if (tile_dim.Y == 0)
        tile_dim.Y = 1;

    img::Image *img = new img::Image(img::PF_RGBA8, tile_dim.X, tile_dim.Y);

    v2u vdim = tile_dim;
    rectu rect(v2u(x0 * vdim.X, y0 * vdim.Y), tile_dim);
    texgen->imgMdf->copyTo(dest, img, &rect, nullptr, true);

    // Replace baseimg
    delete dest;
    dest = img;

    return true;
}

/*
    [png:base64
    Decodes a PNG image in base64 form.
    Use minetest.encode_png and minetest.encode_base64
    to produce a valid string.
*/
bool TexModParser::parsePNG(TextureGenerator *texgen, img::Image *dest, const std::string &mod)
{
    std::string png;
    {
        auto blob = mod.substr(5);
        if (!base64_is_valid(blob)) {
            errorstream << "generateImagePart(): "
                        << "malformed base64 in [png" << std::endl;
            return false;
        }
        png = base64_decode(blob);
    }

    u8 *png_bytes = reinterpret_cast<u8 *>(&png);

    img::Image *png_img = img::ImageLoader::loadFromMem(png_bytes, png.size());

    if (!png_img) {
        errorstream << "TexModParser::parsePNG(): Invalid PNG data" << std::endl;
        return false;
    }

    // blit or use as base
    if (dest) {
        texgen->blitImages(png_img, dest, v2u(), nullptr, true);
        delete png_img;
    } /*else if (pngimg->getColorFormat() != video::ECF_A8R8G8B8) {
        baseimg = driver->createImage(video::ECF_A8R8G8B8, pngimg->getDimension());
        pngimg->copyTo(baseimg);
        pngimg->drop();
    }*/ else {
        dest = png_img;
    }

    return true;
}

/*
    Adjust the hue, saturation, and lightness of destination. Like
    "Hue-Saturation" in GIMP, but with 0 as the mid-point.
    Hue should be from -180 to +180, or from 0 to 360.
    Saturation and Lightness are percentages.
    Lightness is from -100 to +100.
    Saturation goes down to -100 (fully desaturated) but can go above 100,
    allowing for even muted colors to become saturated.

    If colorize is true then saturation is from 0 to 100, and destination will
    be converted to a grayscale image as seen through a colored glass, like
    "Colorize" in GIMP.
*/
static void apply_hue_saturation(img::ImageModifier *mdf, img::Image *dst, v2u dst_pos, v2u size,
    s32 hue, s32 saturation, s32 lightness, bool colorize)
{
    img::colorf colorf;
    img::ColorHSL hsl;
    f32 norm_s = std::clamp(saturation, -100, 1000) / 100.0f;
    f32 norm_l = std::clamp(lightness,  -100, 100) / 100.0f;

    if (colorize) {
        hsl.S = std::clamp((f32)saturation, 0.0f, 100.0f);
    }

    for (u32 y = dst_pos.Y; y < dst_pos.Y + size.Y; y++)
        for (u32 x = dst_pos.X; x < dst_pos.X + size.X; x++) {

            if (colorize) {
                f32 lum = mdf->getPixelColor(dst, x, y).getLuminance() / 255.0f;

                if (norm_l < 0) {
                    lum *= norm_l + 1.0f;
                } else {
                    lum = lum * (1.0f - norm_l) + norm_l;
                }
                hsl.H = 0;
                hsl.L = lum * 100;

            } else {
                // convert the RGB to HSL
                colorf = img::color8ToColorf(mdf->getPixelColor(dst, x, y));
                hsl.fromRGBA(colorf);

                if (norm_l < 0) {
                    hsl.L *= norm_l + 1.0f;
                } else{
                    hsl.L = hsl.L + norm_l * (100.0f - hsl.L);
                }

                // Adjusting saturation in the same manner as lightness resulted in
                // muted colors being affected too much and bright colors not
                // affected enough, so I'm borrowing a leaf out of gimp's book and
                // using a different scaling approach for saturation.
                // https://github.com/GNOME/gimp/blob/6cc1e035f1822bf5198e7e99a53f7fa6e281396a/app/operations/gimpoperationhuesaturation.c#L139-L145=
                // This difference is why values over 100% are not necessary for
                // lightness but are very useful with saturation. An alternative UI
                // approach would be to have an upper saturation limit of 100, but
                // multiply positive values by ~3 to make it a more useful positive
                // range scale.
                hsl.S *= norm_s + 1.0f;
                hsl.S = std::clamp(hsl.S, 0.0f, 100.0f);
            }

            // Apply the specified HSL adjustments
            hsl.H = fmod(hsl.H + hue, 360);
            if (hsl.H < 0)
                hsl.H += 360;

            // Convert back to RGB
            hsl.toRGBA(colorf);
            mdf->setPixelColor(dst, x, y, img::colorfToColor8(colorf));
        }
}

/*
    [hsl:hue:saturation:lightness
    or
    [colorizehsl:hue:saturation:lightness

    Adjust the hue, saturation, and lightness of the base image. Like
    "Hue-Saturation" in GIMP, but with 0 as the mid-point.
    Hue should be from -180 to +180, though 0 to 360 is also supported.
        Saturation and lightness are optional, with lightness from -100 to
    +100, and sauration from -100 to +100-or-higher.

    If colorize is true then saturation is from 0 to 100, and the image
    will be converted to a grayscale image as though seen through a
    colored glass, like	"Colorize" in GIMP.
*/
bool TexModParser::parseHSL(TextureGenerator *texgen, img::Image *dest, const std::string &mod)
{
    bool colorize = str_starts_with(mod, "[colorizehsl:");

    // saturation range is 0 to 100 when colorize is true
    s32 defaultSaturation = colorize ? 50 : 0;

    Strfnd sf(mod);
    sf.next(":");
    s32 hue = mystoi(sf.next(":"), -180, 360);
    s32 saturation = sf.at_end() ? defaultSaturation : mystoi(sf.next(":"), -100, 1000);
    s32 lightness  = sf.at_end() ? 0 : mystoi(sf.next(":"), -100, 100);


    apply_hue_saturation(texgen->imgMdf, dest, v2u(0, 0), dest->getSize(), hue, saturation, lightness, colorize);

    return true;
}

/*
    [overlay:filename
    or
    [hardlight:filename

    "A.png^[hardlight:B.png" is the same as "B.png^[overlay:A.Png"

    Applies an Overlay or Hard Light blend between two images, like the
    layer modes of the same names in GIMP.
    Overlay combines Multiply and Screen blend modes. The parts of the
    top layer where the base layer is light become lighter, the parts
    where the base layer is dark become darker. Areas where the base
    layer are mid grey are unaffected. An overlay with the same picture
    looks like an S-curve.

    Swapping the top layer and base layer is a Hard Light blend
*/
bool TexModParser::parseOverlay(TextureGenerator *texgen, img::Image *dest, const std::string &mod)
{
    Strfnd sf(mod);
    sf.next(":");
    std::string filename = unescape_string(sf.next_esc(":", '\\'));

    img::Image *img = texgen->generate(filename);
    if (img) {
        texgen->upscaleToLargest(dest, img);

        texgen->imgMdf->setBlendMode(img::BM_OVERLAY);
        texgen->imgMdf->copyTo(img, dest, nullptr, nullptr);

        delete img;
    } else {
        errorstream << "generateImage(): Failed to load image \""
                    << filename << "\" for [overlay or [hardlight" << std::endl;
    }

    return true;
}

bool TexModParser::parseHardlight(TextureGenerator *texgen, img::Image *dest, const std::string &mod)
{
    Strfnd sf(mod);
    sf.next(":");
    std::string filename = unescape_string(sf.next_esc(":", '\\'));

    img::Image *img = texgen->generate(filename);
    if (img) {
        texgen->upscaleToLargest(dest, img);

        texgen->imgMdf->setBlendMode(img::BM_HARD_LIGHT);
        texgen->imgMdf->copyTo(img, dest, nullptr, nullptr);

        delete img;
    } else {
        errorstream << "generateImage(): Failed to load image \""
                    << filename << "\" for [overlay or [hardlight" << std::endl;
    }

    return true;
}

/*
    Adjust the brightness and contrast of the base image.

    Conceptually like GIMP's "Brightness-Contrast" feature but allows brightness to be
    wound all the way up to white or down to black.
*/
static void apply_brightness_contrast(img::ImageModifier *mdf, img::Image *dst, v2u dst_pos, v2u size,
        s32 brightness, s32 contrast)
{
    img::color8 dst_c;
    // Only allow normalized contrast to get as high as 127/128 to avoid infinite slope.
    // (we could technically allow -128/128 here as that would just result in 0 slope)
    double norm_c = std::clamp(contrast,   -127, 127) / 128.0;
    double norm_b = std::clamp(brightness, -127, 127) / 127.0;

    // Scale brightness so its range is -127.5 to 127.5, otherwise brightness
    // adjustments will outputs values from 0.5 to 254.5 instead of 0 to 255.
    double scaled_b = brightness * 127.5 / 127;

    // Calculate a contrast slope such that that no colors will get clamped due
    // to the brightness setting.
    // This allows the texture modifier to used as a brightness modifier without
    // the user having to calculate a contrast to avoid clipping at that brightness.
    double slope = 1 - fabs(norm_b);

    // Apply the user's contrast adjustment to the calculated slope, such that
    // -127 will make it near-vertical and +127 will make it horizontal
    double angle = atan(slope);
    angle += norm_c <= 0
                 ? norm_c * angle // allow contrast slope to be lowered to 0
                 : norm_c * (M_PI_2 - angle); // allow contrast slope to be raised almost vert.
    slope = tan(angle);

    double c = slope <= 1
                   ? -slope * 127.5 + 127.5 + scaled_b    // shift up/down when slope is horiz.
                   : -slope * (127.5 - scaled_b) + 127.5; // shift left/right when slope is vert.

    // add 0.5 to c so that when the final result is cast to int, it is effectively
    // rounded rather than trunc'd.
    c += 0.5;

    for (u32 y = dst_pos.Y; y < dst_pos.Y + size.Y; y++)
        for (u32 x = dst_pos.X; x < dst_pos.X + size.X; x++) {
            dst_c = mdf->getPixelColor(dst, x, y);

            dst_c.R(slope * dst_c.R()   + c);
            dst_c.G(slope * dst_c.G()   + c);
            dst_c.B(slope * dst_c.B()   + c);

            mdf->setPixelColor(dst, x, y, dst_c);
        }
}

/*
    [contrast:C:B

    Adjust the brightness and contrast of the base image. Conceptually
    like GIMP's "Brightness-Contrast" feature but allows brightness to
    be wound all the way up to white or down to black.
    C and B are both values from -127 to +127.
    B is optional.
*/
bool TexModParser::parseContrast(TextureGenerator *texgen, img::Image *dest, const std::string &mod)
{
    Strfnd sf(mod);
    sf.next(":");
    s32 contrast = mystoi(sf.next(":"), -127, 127);
    s32 brightness = sf.at_end() ? 0 : mystoi(sf.next(":"), -127, 127);

    apply_brightness_contrast(texgen->imgMdf, dest, v2u(0, 0), dest->getSize(), brightness, contrast);

    return true;
}

u32 TexModParser::parseImageTransform(const std::string &transform_s)
{
    u32 total_transform = 0;

    std::string transform_names[8];
    transform_names[0] = "i";
    transform_names[1] = "r90";
    transform_names[2] = "r180";
    transform_names[3] = "r270";
    transform_names[4] = "fx";
    transform_names[6] = "fy";

    std::size_t pos = 0;
    while(pos < transform_s.size())
    {
        int transform = -1;
        for (int i = 0; i <= 7; ++i)
        {
            const std::string &name_i = transform_names[i];

            if (transform_s[pos] == ('0' + i))
            {
                transform = i;
                pos++;
                break;
            }

            if (!(name_i.empty()) && lowercase(transform_s.substr(pos, name_i.size())) == name_i) {
                transform = i;
                pos += name_i.size();
                break;
            }
        }
        if (transform < 0)
            break;

        // Multiply total_transform and transform in the group D4
        int new_total = 0;
        if (transform < 4)
            new_total = (transform + total_transform) % 4;
        else
            new_total = (transform - total_transform + 8) % 4;
        if ((transform >= 4) ^ (total_transform >= 4))
            new_total += 4;

        total_transform = new_total;
    }

    return total_transform;
}
