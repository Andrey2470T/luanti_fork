#include "texture_modifiers.h"
#include "util/numeric.h"
#include "util/string.h"
#include "log.h"
#include "settings.h"

bool TexModParser::determineModifier(TextureGenerator *texgen, img::Image *dest, const std::string &mod)
{
    if (str_starts_with(mod, "crack"))
        return parseCrack(texgen, dest, mod);
    else if (str_starts_with(mod, "combine"))
        return parseCombine(texgen, dest, mod);
    else if (str_starts_with(mod, "fill"))
        return parseFill(texgen, dest, mod);
    else if (str_starts_with(mod, "brighten"))
        return parseBrighten(texgen, dest, mod);
    else if (str_starts_with(mod, "noalpha"))
        return parseNoAlpha(texgen, dest, mod);
    else if (str_starts_with(mod, "makealpha"))
        return parseMakeAlpha(texgen, dest, mod);
    else if (str_starts_with(mod, "transform"))
        return parseTransform(texgen, dest, mod);
    else if (str_starts_with(mod, "inventorycube"))
        return parseInventoryCube(texgen, dest, mod);
    else if (str_starts_with(mod, "lowpart"))
        return parseLowPart(texgen, dest, mod);
    else if (str_starts_with(mod, "verticalframe"))
        return parseVerticalFrame(texgen, dest, mod);
    else if (str_starts_with(mod, "mask"))
        return parseMask(texgen, dest, mod);
    else if (str_starts_with(mod, "multiply"))
        return parseMultiply(texgen, dest, mod);
    else if (str_starts_with(mod, "screen"))
        return parseScreen(texgen, dest, mod);
    else if (str_starts_with(mod, "colorize"))
        return parseColorize(texgen, dest, mod);
    else if (str_starts_with(mod, "applyfiltersformesh"))
        return parseApplyFiltersForMesh(texgen, dest, mod);
    else if (str_starts_with(mod, "resize"))
        return parseResize(texgen, dest, mod);
    else if (str_starts_with(mod, "opacity"))
        return parseOpacity(texgen, dest, mod);
    else if (str_starts_with(mod, "invert"))
        return parseInvert(texgen, dest, mod);
    else if (str_starts_with(mod, "sheet"))
        return parseSheet(texgen, dest, mod);
    else if (str_starts_with(mod, "png"))
        return parsePNG(texgen, dest, mod);
    else if (str_starts_with(mod, "hsl"))
        return parseHSL(texgen, dest, mod);
    else if (str_starts_with(mod, "overlay"))
        return parseOverlay(texgen, dest, mod);
    else if (str_starts_with(mod, "contrast"))
        return parseContrast(texgen, dest, mod);
}

TextureGenerator::TextureGenerator(ResourceCache *cache, img::ImageModifier *mdf)
    : resCache(cache), imgMdf(mdf)
{
    meshFilterNeeded = g_settings->getBool("mip_map") ||
                       g_settings->getBool("trilinear_filter") ||
                       g_settings->getBool("bilinear_filter") ||
                       g_settings->getBool("anisotropic_filter");
}

img::Image *TextureGenerator::generate(const std::string &texmod_str)
{
    // Get the base image

    const char separator = '^';
    const char escape = '\\';
    const char paren_open = '(';
    const char paren_close = ')';

    // Find last separator in the name
    s32 last_separator_pos = -1;
    u8 paren_bal = 0;
    for (s32 i = texmod_str.size() - 1; i >= 0; i--) {
        if (i > 0 && texmod_str[i-1] == escape)
            continue;
        switch (texmod_str[i]) {
        case separator:
            if (paren_bal == 0) {
                last_separator_pos = i;
                i = -1; // break out of loop
            }
            break;
        case paren_open:
            if (paren_bal == 0) {
                errorstream << "TextureGenerator::generate(): unbalanced parentheses"
                    << "(extranous '(') while generating texture \"" << texmod_str << "\"" << std::endl;
                return nullptr;
            }
            paren_bal--;
            break;
        case paren_close:
            paren_bal++;
            break;
        default:
            break;
        }
    }
    if (paren_bal > 0) {
        errorstream << "TextureGenerator::generate(): unbalanced parentheses"
            << "(missing matching '(') while generating texture \"" << texmod_str << "\"" << std::endl;
        return nullptr;
    }


    img::Image *baseimg = nullptr;

    /*
        If separator was found, make the base image
        using a recursive call.
    */
    if (last_separator_pos != -1) {
        baseimg = generate(texmod_str.substr(0, last_separator_pos));
    }

    /*
        Parse out the last part of the name of the image and act
        according to it
    */

    auto last_part_of_name = texmod_str.substr(last_separator_pos + 1);

    /*
        If this name is enclosed in parentheses, generate it
        and blit it onto the base image
    */
    if (last_part_of_name.empty()) {
        // keep baseimg == nullptr
    } else if (last_part_of_name[0] == paren_open
               && last_part_of_name.back() == paren_close) {
        auto texmod_str2 = last_part_of_name.substr(1, last_part_of_name.size() - 2);

        img::Image *tmp = generate(texmod_str2);
        if (!tmp) {
            errorstream << "TextureGenerator::generate(): Failed to generate \"" << texmod_str2 << "\"\n"
                "part of texture \"" << texmod_str << "\""
                        << std::endl;
            return nullptr;
        }

        if (baseimg) {
            blitImagesWithUpscaling(tmp, baseimg);
            delete tmp;
        } else {
            baseimg = tmp;
        }
    } else if (!generatePart(last_part_of_name, baseimg)) {
        // Generate image according to part of name
        errorstream << "TextureGenerator::generate(): Failed to generate \"" << last_part_of_name << "\"\n"
            "part of texture \"" << texmod_str << "\"" << std::endl;
    }

    // If no resulting image, print a warning
    if (baseimg == nullptr) {
        errorstream << "TextureGenerator::generate(): baseimg is NULL (attempted to"
            " create texture \"" << texmod_str << "\")" << std::endl;
    } else if (baseimg->getWidth() == 0 || baseimg->getHeight() == 0) {
        errorstream << "TextureGenerator::generate(): zero-sized image was created?! "
                       "(attempted to create texture \"" << texmod_str << "\")" << std::endl;
        delete baseimg;
        baseimg = nullptr;
    }

    return baseimg;
}

img::Image *TextureGenerator::generateForMesh(const std::string &name)
{
    if (meshFilterNeeded)
        return generate(name + "[applyfiltersformesh");
    return generate(name);
}

// 'texmod_str_part' can be either some image e.g, "default_stone.png" or modifier e.g. "[multiply:red"
bool TextureGenerator::generatePart(const std::string &texmod_str_part, img::Image *base_img)
{
    if (base_img && base_img->getSize().getLengthSQ() == 0) {
        errorstream << "TextureGenerator::generatePart(): baseimg is zero-sized?!"
			<< std::endl;
        delete base_img;
        base_img = nullptr;
    }

    // This is either an image or invalid name
    if (texmod_str_part[0] != '[') {
        img::Image *img = nullptr;
        ResourceInfo *img_cache = resCache->getOrLoad(ResourceType::IMAGE, texmod_str_part);

        if (!img_cache) {
            if (texmod_str_part.empty())
                return true;

            errorstream << "TextureGenerator::generatePart(): Could not load image \""
			    << texmod_str_part << "\" while building texture; "
			    "Creating a dummy image" << std::endl;

            img = createDummyImage();
        }
        else
            img = dynamic_cast<ImageResourceInfo*>(img_cache)->data.get();

        if (!base_img) {
            auto size = img->getSize();
            base_img = new img::Image(img::PF_RGBA8, size.X, size.Y);
            imgMdf->copyTo(img, base_img);
        }
        else
            blitImagesWithUpscaling(img, base_img);
    }
    // Then this is a texture modifier
    else {
        if (!base_img) {
            errorstream << "TextureGenerator::generatePart(): base_img == nullptr" \
					<< " for texmod_str_part\"" << texmod_str_part \
					<< "\", cancelling." << std::endl; \
            return false;
        }

        return TexModParser::determineModifier(this, base_img, texmod_str_part);
    }

    return true;
}

void TextureGenerator::upscaleToLargest(img::Image *img1, img::Image *img2)
{
    auto size1 = img1->getSize();
    auto size2 = img2->getSize();

    if (size1 != size2) {
        if (size1.X < size2.X || size1.Y < size2.Y)
            imgMdf->resize(img1, rectu(size2), img::RF_BICUBIC);
        else
            imgMdf->resize(img2, rectu(size1), img::RF_BICUBIC);
    }
}

img::Image *TextureGenerator::createInventoryCubeImage(
    img::Image *top, img::Image *left, img::Image *right)
{
    auto size_top = top->getSize();
    auto size_left = left->getSize();
    auto size_right = right->getSize();

    // It must be divisible by 4, to let everything work correctly.
    // But it is a power of 2, so being at least 4 is the same.
    // And the resulting texture should't be too large as well.
    u32 size = std::clamp<u32>(npot2(std::max({
        size_top.X, size_top.Y,
        size_left.X, size_left.Y,
        size_right.X, size_right.Y,
    })), 4, 64);

    // With such parameters, the cube fits exactly, touching each image line
    // from `0` to `cube_size - 1`. (Note that division is exact here).
    u32 cube_size = 9 * size;
    u32 offset = size / 2;

    img::Image *res_img = new img::Image(img::PF_RGBA8, cube_size, cube_size);

    // Draws single cube face
    // `shade_factor` is face brightness, in range [0.0, 1.0]
    // (xu, xv, x1; yu, yv, y1) form coordinate transformation matrix
    // `offsets` list pixels to be drawn for single source pixel
    auto draw_image = [=] (img::Image *image, f32 shade_factor,
                          s16 xu, s16 xv, s16 x1,
                          s16 yu, s16 yv, s16 y1,
                          std::initializer_list<v2s16> offsets) -> void {
        u8 brightness = 256 * shade_factor;

        auto img_size = image->getSize();

        if (img_size.X != size || img_size.Y != size)
            imgMdf->resize(image, rectu(v2u(size)), img::RF_BICUBIC);

        for (u32 v = 0; v < size; v++) {
            for (u32 u = 0; u < size; u++) {
                auto color = imgMdf->getPixelColor(image, u, v);
                color = color * brightness / (u8)256;
                s16 x = xu * u + xv * v + x1;
                s16 y = yu * u + yv * v + y1;
                for (const auto &off : offsets) {
                    u32 res_x = x + off.X;
                    u32 res_y = y + off.Y;
                    res_x = res_x + offset >= size ? offset-(size-1-res_x)-1 : res_x + offset;
                    res_y = res_x + offset >= size ? res_y+1 : res_y;
                    imgMdf->setPixelColor(res_img, res_x, res_y, color);
                }
            }
        }
        delete image;
    };

    draw_image(top, 1.000000f,
               4, -4, 4 * (size - 1),
               2, 2, 0,
               {
                {2, 0}, {3, 0}, {4, 0}, {5, 0},
                {0, 1}, {1, 1}, {2, 1}, {3, 1}, {4, 1}, {5, 1}, {6, 1}, {7, 1},
                {2, 2}, {3, 2}, {4, 2}, {5, 2},
                });

    draw_image(left, 0.836660f,
               4, 0, 0,
               2, 5, 2 * size,
               {
                {0, 0}, {1, 0},
                {0, 1}, {1, 1}, {2, 1}, {3, 1},
                {0, 2}, {1, 2}, {2, 2}, {3, 2},
                {0, 3}, {1, 3}, {2, 3}, {3, 3},
                {0, 4}, {1, 4}, {2, 4}, {3, 4},
                {2, 5}, {3, 5},
                });

    draw_image(right, 0.670820f,
               4, 0, 4 * size,
               -2, 5, 4 * size - 2,
               {
                {2, 0}, {3, 0},
                {0, 1}, {1, 1}, {2, 1}, {3, 1},
                {0, 2}, {1, 2}, {2, 2}, {3, 2},
                {0, 3}, {1, 3}, {2, 3}, {3, 3},
                {0, 4}, {1, 4}, {2, 4}, {3, 4},
                {0, 5}, {1, 5},
                });

    return res_img;
}

img::Image *TextureGenerator::createDummyImage()
{
    img::color8 randomColor(img::PF_RGBA8,
        myrand()%256, myrand()%256,myrand()%256
    );

    return new img::Image(img::PF_RGBA8, 1, 1, randomColor);
}

img::Image *TextureGenerator::createCrack(img::Image *img, s32 frame_index,
    v2u size, u8 tiles)
{
    auto strip_size = img->getSize();

    if (tiles == 0 || strip_size.X*strip_size.Y == 0)
        return nullptr;

    v2u frame_size(strip_size.X, strip_size.X);
    v2u tile_size(size / tiles);
    s32 frame_count = strip_size.Y / strip_size.X;
    if (frame_index >= frame_count)
        frame_index = frame_count - 1;
    rectu frame(v2u(0, frame_index * frame_size.Y), frame_size);
    img::Image *result = nullptr;

    // extract crack frame
    img::Image *crack_tile = new img::Image(img::PF_RGBA8, tile_size.X, tile_size.Y);
    if (!crack_tile)
        return nullptr;
    if (tile_size == frame_size) {
        imgMdf->copyTo(img, crack_tile, nullptr, &frame);
    } else {
        img::Image *crack_frame = new img::Image(img::PF_RGBA8, frame_size.X, frame_size.Y);

        imgMdf->copyTo(img, crack_frame, nullptr, &frame);
        imgMdf->copyTo(crack_frame, crack_tile);
        delete crack_frame;
    }
    if (tiles == 1)
        return crack_tile;

    // tile it
    result = new img::Image(img::PF_RGBA8, size.X, size.Y);

    for (u8 i = 0; i < tiles; i++)
        for (u8 j = 0; j < tiles; j++) {
            rectu rect_i(v2u(i * tile_size.X, j * tile_size.Y));
            imgMdf->copyTo(crack_tile, result, &rect_i);
        }

    delete crack_tile;
    return result;
}

void TextureGenerator::blitImagesWithUpscaling(img::Image *src, img::Image *dest)
{
    upscaleToLargest(src, dest);

    for (u32 x = 0; x < dest->getWidth(); x++)
        for (u32 y = 0; y < dest->getHeight(); y++) {
            auto pixel = doBlend(imgMdf->getPixelColor(src, x, y), imgMdf->getPixelColor(dest, x, y), img::BM_ALPHA);
            imgMdf->setPixelColor(dest, x, y, pixel);
        }
}
