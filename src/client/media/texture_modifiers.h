#pragma once

#include <Image/ImageModifier.h>


#define PARSE_FUNC(MODIFIER_NAME) \
    static bool parse##MODIFIER_NAME(TextureGenerator *texgen, img::Image *dest, const std::string &mod)

class TextureGenerator;
class ResourceCache;

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
    PARSE_FUNC(Hardlight);
    PARSE_FUNC(Contrast);

    static u32 parseImageTransform(const std::string &transform_s);
};

class TextureGenerator
{
    ResourceCache *resCache;
    img::ImageModifier *imgMdf;

    bool meshFilterNeeded;
    u16 minTextureSize;

    friend TexModParser;
public:
    TextureGenerator(ResourceCache *cache, img::ImageModifier *mdf);

    img::Image *generate(const std::string &texmod_str);
    img::Image *generateForMesh(const std::string &name);

    ResourceCache *getResourceCache() const
    {
        return resCache;
    }
    img::ImageModifier *getImageModifier() const
    {
        return imgMdf;
    }
private:
    bool generatePart(const std::string &texmod_str_part, img::Image *base_img);

    void upscaleToLargest(img::Image *img1, img::Image *img2);

    img::Image *createInventoryCubeImage(
        img::Image *img1, img::Image *img2, img::Image *img3);

    img::Image *createCrack(img::Image *img, s32 frame_index,
		v2u size, u8 tiles);
    
    void blitImages(img::Image *src, img::Image *dest, const v2u &dstPos=v2u(0, 0), const v2u *srcSize=nullptr, bool upscale=false);
};
