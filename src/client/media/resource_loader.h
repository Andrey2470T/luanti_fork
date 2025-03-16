#pragma once

#include "BasicIncludes.h"
#include "resource.h"

class ResourceLoader
{
public:
	img::Image *loadImage(const std::string &name);
	render::Texture2D *loadTexture(const std::string &name);
	render::Shader *loadShader(const std::string &name);
	MeshBuffer *loadMesh(const std::strint &name);
	
};