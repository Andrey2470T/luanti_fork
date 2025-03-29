#pragma once

#include "Image/ImageModifier.h"
#include "resource_loader.h"

class TextureGenerator
{
public:
    TextureGenerator();

    render::Texture2D *generate(const std::string &texmod_str);
    render::Texture2D *generateForMesh(const std::string &name);
private:
    bool generatePart(const std::string &texmod_str_part, img::Image *base_img);
};