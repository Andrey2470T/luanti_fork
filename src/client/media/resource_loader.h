#pragma once

#include "BasicIncludes.h"
#include "resource.h"

class ResourceLoader
{
public:
	img::Image *loadImage(const std::string &path);
	render::Texture2D *loadTexture(const std::string &path);
	render::Shader *loadShader(const std::string &path);
	MeshBuffer *loadMesh(const std::strint &path);
	
};