#pragma once

#include "Image/ImageModifier.h"
#include "resource.h"

#define PARSE_FUNC(MODIFIER_NAME) \
    static bool parse##MODIFIER_NAME(TextureGenerator *texgen, img::Image *dest, const std::string &mod)

class TextureGenerator;

class TexModParser
{
public:
    static bool determineModifier(TextureGenerator *texgen, img::Image *dest, const std::string &mod);
private:
    PARSE_FUNC(Crack);
    PARSE_FUNC(Combine);
    PARSE_FUNC(Fill);
    PARSE_FUNC(Brighten);
    PARSE_FUNC(NoAlpha);
    PARSE_FUNC(MakeAlpha);
    PARSE_FUNC(Transform);
    PARSE_FUNC(InventoryCube);
    PARSE_FUNC(LowPart);
    PARSE_FUNC(VerticalFrame);
    PARSE_FUNC(Mask);
    PARSE_FUNC(Multiply);
    PARSE_FUNC(Screen);
    PARSE_FUNC(Colorize);
    PARSE_FUNC(ApplyFiltersForMesh);
    PARSE_FUNC(Resize);
    PARSE_FUNC(Opacity);
    PARSE_FUNC(Invert);
    PARSE_FUNC(Sheet);
    PARSE_FUNC(PNG);
    PARSE_FUNC(HSL);
    PARSE_FUNC(Overlay);
    PARSE_FUNC(Contrast);
};

class TextureGenerator
{
    ResourceCache *resCache;
    img::ImageModifier *imgMdf;

    bool meshFilterNeeded;

    friend TexModParser;
public:
    TextureGenerator(ResourceCache *cache, img::ImageModifier *mdf);

    img::Image *generate(const std::string &texmod_str);
    img::Image *generateForMesh(const std::string &name);
private:
    bool generatePart(const std::string &texmod_str_part, img::Image *base_img);

    void upscaleToLargest(img::Image *img1, img::Image *img2);

    img::Image *createInventoryCubeImage(
        img::Image *img1, img::Image *img2, img::Image *img3);
    
    img::Image *createDummyImage();

    img::Image *createCrack(img::Image *img, s32 frame_index,
		v2u size, u8 tiles);
    
    void blitImagesWithUpscaling(img::Image *src, img::Image *dest);
};
