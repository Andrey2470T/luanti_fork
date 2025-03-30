#include "texture_modifiers.h"
#include "util/numeric.h"
#include "util/string.h"

static std::vector<std::string> strModifiers = {
    "[crack",
    "[combine",
    "[fill",
    "[brighten",
    "[noalpha",
    "[makealpha:",
    "[transform",
    "[inventorycube",
    "[lowpart:",
    "[verticalframe:",
    "[mask:",
    "[multiply:",
    "[screen:",
    "[colorize:",
    "[applyfiltersformesh",
    "[resize",
    "[opacity:",
    "[invert:",
    "[sheet:",
    "[png:",
    "[hsl:",
    "[overlay:",
    "[contrast:"
};

TextureGenerator::TextureGenerator(ResourceCache *cache, img::ImageModifier *mdf)
    : resCache(cache), imgMdf(mdf)
{}

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
        ResourceCache *img_cache = resCache->getOrLoad(ResourceType::IMAGE, texmod_str_part);

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
            baseImg = new Image(img::PF_RGBA8, size.X, size.Y);
            imgMdf->copyTo(img, baseImg);
        }
        else
            blitImagesWithUpscaling(img, baseImg);
    }
    // Then this is a texture modifier
    else {
        if (!base_img) {
            errorstream << "TextureGenerator::generatePart(): base_img == nullptr" \
					<< " for texmod_str_part\"" << texmod_str_part \
					<< "\", cancelling." << std::endl; \
            return false;
        }
    }
}

img::Image *TextureGenerator::createDummyImage()
{
    img::color8 randomColor(img::PF_RGBA8,
        myrand()%256, myrand()%256,myrand()%256
    );

    return new Image(img::PF_RGBA8, 1, 1, randomColor);
}