#include "resource.h"
#include "FilesystemVersions.h"
#include "porting.h"
#include "threading/mutex_auto_lock.h"
#include "settings.h"
#include "Image/ImageLoader.h"

static std::vector<std::string> getTexturesDefaultPaths()
{
    std::vector<std::string> paths;
    for (auto &entry : fs::recursive_directory_iterator(g_settings->get("texture_path")))
        paths.push_back(entry.path().string());

    fs::path basePath = porting::path_share;
    basePath /= "textures";
    basePath /= "base";
    basePath /= "pack";

    paths.push_back(basePath.string());

    return paths;
}

static std::vector<std::string> getShaderDefaultPaths()
{
    std::vector<std::string> paths;
    paths.push_back(g_settings->get("shader_path"));

    fs::path basePath = porting::path_share;
    basePath /= "client";
    basePath /= "shaders";

    paths.push_back(basePath.string());

    return paths;
}

std::vector<std::string> TextureResourceInfo::defpaths = getTexturesDefaultPaths();
std::vector<std::string> ImageResourceInfo::defpaths = TextureResourceInfo::defpaths;
std::vector<std::string> ShaderResourceInfo::defpaths = getShaderDefaultPaths();

ResourceCache::ResourceCache(img::ImageModifier *mdf)
    : loader(std::make_unique<ResourceLoader>(mdf))
{}

ResourceInfo *ResourceCache::get(ResourceType _type, const std::string &_name)
{
    MutexAutoLock lock(resource_mutex);
	switch(_type) {
		case ResourceType::TEXTURE: {
			auto it = std::find(textures.begin(), textures.end(), [_name] (const std::unique_ptr<TextureResourceInfo> &elem) {
				return elem->name == _name;
			});
			
            return (it != textures.end() ? it->get() : nullptr);
		}
		case ResourceType::IMAGE: {
			auto it = std::find(images.begin(), images.end(), [_name] (const std::unique_ptr<ImageResourceInfo> &elem) {
				return elem->name == _name;
			});
			
            return (it != images.end() ? it->get() : nullptr);
		}
		case ResourceType::SHADER: {
			auto it = std::find(shaders.begin(), shaders.end(), [_name] (const std::unique_ptr<ShaderResourceInfo> &elem) {
				return elem->name == _name;
			});
			
            return (it != shaders.end() ? it->get() : nullptr);
		}
        case ResourceType::MESH: {
			auto it = std::find(meshes.begin(), meshes.end(), [_name] (const std::unique_ptr<MeshResourceInfo> &elem) {
				return elem->name == _name;
			});
			
            return (it != meshes.end() ? it->get() : nullptr);
		}
        case ResourceType::PALETTE: {
            auto it = std::find(palettes.begin(), palettes.end(), [_name] (const std::unique_ptr<PaletteResourceInfo> &elem) {
                return elem->name == _name;
            });

            return (it != palettes.end() ? it->get() : nullptr);
        }
	};
}

ResourceInfo *ResourceCache::getByID(ResourceType _type, u32 _id)
{
	MutexAutoLock resource_lock(resource_mutex);

    switch (_type) {
    case ResourceType::TEXTURE: {
        if (_id >= textures.size()) {
            infostream << "ResourceCache::getResourceByID(): Texture resource ID " << _id << " is out of range" << std::endl;
            return nullptr;
        }
        else
            return textures.at(_id).get();
    }
    case ResourceType::IMAGE: {
        if (_id >= images.size()) {
            infostream << "ResourceCache::getResourceByID(): Image resource ID " << _id << " is out of range" << std::endl;
            return nullptr;
        }
        else
            return images.at(_id).get();
    }
    case ResourceType::SHADER: {
        if (_id >= shaders.size()) {
            infostream << "ResourceCache::getResourceByID(): Shader resource ID " << _id << " is out of range" << std::endl;
            return nullptr;
        }
        else
            return shaders.at(_id).get();
    }
    case ResourceType::MESH: {
        if (_id >= meshes.size()) {
            infostream << "ResourceCache::getResourceByID(): Mesh resource ID " << _id << " is out of range" << std::endl;
            return nullptr;
        }
        else
            return meshes.at(_id).get();
    }
    case ResourceType::PALETTE: {
        if (_id >= palettes.size()) {
            infostream << "ResourceCache::getResourceByID(): Palette resource ID " << _id << " is out of range" << std::endl;
            return nullptr;
        }
        else
            return palettes.at(_id).get();
    }
    }
}

ResourceInfo *ResourceCache::getOrLoad(ResourceType _type, const std::string &_name)
{
	ResourceInfo *res = get(_type, _name);

    // The resource is already cached
	if (res)
		return res;

	std::string target_path;

	switch(_type) {
		case ResourceType::TEXTURE: {
			target_path = findFirstExistentDefaultPath(TextureResourceInfo::defpaths, _name, _type);
			break;
		}
		case ResourceType::IMAGE: {
			target_path = findFirstExistentDefaultPath(ImageResourceInfo::defpaths, _name, _type);
			break;
		}
		case ResourceType::SHADER: {
			target_path = findFirstExistentDefaultPath(ShaderResourceInfo::defpaths, _name, _type);
			break;
		}
		default:
			break;
	}



    if (target_path.empty()) {
        fs::path abs_p = fs::absolute(_name);

        if (fs::exists(abs_p))
            target_path = abs_p.parent_path().string();
        else {
            infostream << "ResourceCache::getOrLoad(): Couldn't load the resource with name " << _name << " from any known source path" << std::endl;
            return nullptr;
        }
    }

	MutexAutoLock resource_lock(resource_mutex);
	switch(_type) {
		case ResourceType::TEXTURE: {
            render::Texture2D *tex = loader->loadTexture(target_path);

			if (!tex) {
                infostream << "ResourceCache::getOrLoad(): Couldn't load the texture with name " << _name << std::endl;
				return nullptr;
			}

			textures.emplace_back(
				ResourceType::TEXTURE,
				_name,
				target_path,
				tex
			);
			return textures.back().get();
		}
		case ResourceType::IMAGE: {
            img::Image *img = loader->loadImage(target_path);

			if (!img) {
                infostream << "ResourceCache::getOrLoad(): Couldn't load the image with name " << _name << std::endl;
				return nullptr;
			}

			images.emplace_back(
				ResourceType::IMAGE,
				_name,
				target_path,
				img
			);
			return images.back().get();
		}
		case ResourceType::SHADER: {
            render::Shader *shader = loader->loadShader(target_path);

			if (!shader) {
                infostream << "ResourceCache::getOrLoad(): Couldn't load the shader with name " << _name << std::endl;
				return nullptr;
			}

			shaders.emplace_back(
				ResourceType::SHADER,
				_name,
				target_path,
				shader
			);
			return shaders.back().get();
		}
		case ResourceType::MESH: {
            MeshBuffer *mesh = loader->loadMesh(target_path);

			if (!mesh) {
                infostream << "ResourceCache::getOrLoad(): Couldn't load the mesh with name " << _name << std::endl;
				return nullptr;
			}

			meshes.emplace_back(
				ResourceType::MESH,
				_name,
				target_path,
				mesh
			);
			return meshes.back().get();
		}
        case ResourceType::PALETTE: {
            img::Palette *palette = loader->loadPalette(target_path);

            if (!palette) {
                infostream << "ResourceCache::getOrLoad(): Couldn't load the palette image with name " << _name << std::endl;
                return nullptr;
            }

            palettes.emplace_back(
                ResourceType::PALETTE,
                _name,
                target_path,
                palette
                );
            return palettes.back().get();
        }
		default:
			return nullptr;
	};
}

std::string ResourceCache::findFirstExistentDefaultPath(const std::vector<std::string> &defpaths, const std::string &name, ResourceType type)
{
	for (auto &p : defpaths) {
		if (type == ResourceType::IMAGE || type == ResourceType::TEXTURE) {
            for (auto &ext : img::SIE) {
                fs::path fs_p(fs::path(p) / (name + ext));
				if (fs::exists(fs_p))
					return p;
			}
		}
		else if (type == ResourceType::SHADER) {
            fs::path vs_p(fs::path(p) / name / "opengl_vertex.glsl");
            fs::path fs_p(fs::path(p) / name / "opengl_fragment.glsl");
			
			if (fs::exists(vs_p) && fs::exists(fs_p))
			    return p;
        }
		else {
            fs::path fs_p(fs::path(p) / name);
			if (fs::exists(fs_p))
				return p;
		}
	}

	return "";
}
