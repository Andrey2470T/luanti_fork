#include "texture_modifiers.h"
#include "util/strfnd.h"
#include "util/string.h"
#include "log.h"

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

bool TexModParser::parseCrack(TextureGenerator *texgen, img::Image *dest, const std::string &mod)
{
    // Crack image number and overlay option
    // Format: crack[o][:<tiles>]:<frame_count>:<frame>
    bool use_overlay = (mod[6] == 'o');
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
        img::Image *img_crack = dynamic_cast<ImageResourceInfo*>(texgen->resCache->getOrLoad(
                ResourceType::IMAGE, "crack_anylength.png"))->data.get();

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

        for (s32 i = 0; i < frame_count; ++i) {
            v2u dst_pos(0, frame_size.Y * i);
            texgen->blitImagesWithUpscaling(crack_scaled, dest);
        }

        delete crack_scaled;
        /*draw_crack(img_crack, baseimg,
                use_overlay, frame_count,
                progression, driver, tiles);
        img_crack->drop();*/
    }

    return true;
}

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
        v2i pos_base;
        pos_base.X = stoi(sf.next(","));
        pos_base.Y = stoi(sf.next("="));
        std::string filename = unescape_string(sf.next_esc(":", '\\'));

        auto basedim = dest->getSize();
        if (pos_base.X > (s32)basedim.X || pos_base.Y > (s32)basedim.Y) {
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

        texgen->blitImagesWithUpscaling(img, dest);
        delete img;
    }

    return true;
}

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
    v2u dim(width, height);

    CHECK_DIM(width, height, mod);
    if (dest) {
        auto basedim = dest->getSize();
        if (x >= basedim.X || y >= basedim.Y)
            COMPLAIN_INVALID("X or Y offset", mod);
    }

    img::Image *img = new img::Image(img::PF_RGBA8, dim.X, dim.Y, color);

    if (!dest) {
        dest = img;
    } else {
        texgen->blitImagesWithUpscaling(img, dest);
        delete img;
    }

    return true;
}
