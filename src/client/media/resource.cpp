#include "resource.h"
#include "FilesystemVersions.h"
#include "porting.h"
#include <system_error>
#include "threading/mutex_auto_lock.h"

static std::vector<std::string> getTexturesDefaultPaths()
{
    std::vector<std::string> paths;
    for (auto &entry : fs::recursive_directory_iterator(g_settings->get("texture_path")))
        paths.push_back(entry.path().string());

    fs::path basePath = porting::path_share;

    basePath /= "textures" /= "base" /= "pack";
    paths.push_back(basePath.string());

    return paths;
}

static std::vector<std::string> getShaderDefaultPaths()
{
    std::vector<std::string> paths;
    paths.push_back(g_settings->get("shader_path"));

    fs::path basePath = porting::path_share;
    basePath /= "client" /= "shaders";
    paths.push_back(basePath.string());

    return paths;
}

std::vector<std::string> TextureResourceInfo::defpaths = getTexturesDefaultPaths();
std::vector<std::string> ImageResourceInfo::defpaths = TextureResourceInfo::defpaths;
std::vector<std::string> ShaderResourceInfo::defpaths = getShaderDefaultPaths();

ResourceInfo *ResourceCache::get(ResourceType _type, const std::string &_name) const
{
	MutexAutoLock resource_lock(resource_mutex);
	switch(_type) {
		case ResourceType::TEXTURE: {
			auto it = std::find(textures.begin(), textures.end(), [_name] (const std::unique_ptr<TextureResourceInfo> &elem) {
				return elem->name == _name;
			});
			
			return (it != textures.end() ? &it->second.get() : nullptr);
		}
		case ResourceType::IMAGE: {
			auto it = std::find(images.begin(), images.end(), [_name] (const std::unique_ptr<ImageResourceInfo> &elem) {
				return elem->name == _name;
			});
			
			return (it != images.end() ? &it->second.get() : nullptr);
		}
		case ResourceType::SHADER: {
			auto it = std::find(shaders.begin(), shaders.end(), [_name] (const std::unique_ptr<ShaderResourceInfo> &elem) {
				return elem->name == _name;
			});
			
			return (it != shaders.end() ? &it->second.get() : nullptr);
		}
		case ResourceType::TEXTURE: {
			auto it = std::find(meshes.begin(), meshes.end(), [_name] (const std::unique_ptr<MeshResourceInfo> &elem) {
				return elem->name == _name;
			});
			
			return (it != meshes.end() ? &it->second.get() : nullptr);
		}
	};
}

ResourceInfo *ResourceCache::getByID(ResourceType _type, u32 _id) const
{
	MutexAutoLock resource_lock(resource_mutex);
	switch(_type) {
		case ResourceType::TEXTURE:
			return getResourceByID(dynamic_cast<std::vector<ResourceInfo>>(textures), _id);
		case ResourceType::IMAGE:
			return getResourceByID(dynamic_cast<std::vector<ResourceInfo>>(images), _id);
		case ResourceType::SHADER:
			return getResourceByID(dynamic_cast<std::vector<ResourceInfo>>(shaders), _id);
		case ResourceType::MESH:
			return getResourceByID(dynamic_cast<std::vector<ResourceInfo>>(meshes), _id);
	}
}

ResourceInfo *ResourceCache::getOrLoad(ResourceType _type, const std::string &_name)
{
	ResourceInfo *res = get(_type, _name);

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

	if (!target_path.empty())
		return target_path;

	std::error_code err;
	fs::path abs_p = fs::absolute(_name, err);

	if (fs::exists(abs_p))
		target_path = abs_p.parent_path().string();
	else {
		infostream << "ResourceCache::getOrLoad(): Couldn't load the resource with name " << _name << " from any known source path" << std::endl;
		return nullptr;
	}

	MutexAutoLock resource_lock(resource_mutex);
	switch(_type) {
		case ResourceType::TEXTURE: {
			render::Texture2D *tex = loader.loadTexture(target_path);

			if (!tex) {
				infostream << "ResourceCache::getOrLoad(): Couldn't load the texture with name " << name << std::endl;
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
			img::Image *img = loader.loadImage(target_path);

			if (!img) {
				infostream << "ResourceCache::getOrLoad(): Couldn't load the image with name " << name << std::endl;
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
			render::Shader *shader = loader.loadShader(target_path);

			if (!shader) {
				infostream << "ResourceCache::getOrLoad(): Couldn't load the shader with name " << name << std::endl;
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
			MeshBuffer *mesh = loader.loadMesh(target_path);

			if (!mesh) {
				infostream << "ResourceCache::getOrLoad(): Couldn't load the mesh with name " << name << std::endl;
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
		default:
			return nullptr;
	};
}

ResourceInfo *ResourceCache::getResourceByID(const std::vector<std::unique_ptr<ResourceInfo>> &resources, u32 id) const
{
	if (id >= resources.size()) {
		infostream << "ResourceCache::getResourceByID(): Resource ID " << _id << " is out of range" << std::endl;
		return nullptr;
	}
	else
		return resources.at(id).get();
}

std::string ResourceCache::findFirstExistentDefaultPath(const std::vector<std::string> &defpaths, const std::string &name, ResourceType type)
{
	for (auto &p : defpaths) {
		if (type == ResourceType::IMAGE || type == ResourceType::TEXTURE) {
			for (auto &ext : img::SIE) {
				fs::path fs_p(p / (name + ext));
				if (fs::exists(fs_p))
					return p;
			}
		}
		else if (type == ResourceType::SHADER) {
			fs::path vs_p(p / name / "opengl_vertex.glsl");
			fs::path fs_p(p / name / "opengl_fragment.glsl");
			
			if (fs::exists(vs_p) && fs::exists(fs_p))
			    return p;
        }
		else {
			fs::path fs_p(p / name);
			if (fs::exists(fs_p))
				return p;
		}
	}

	return "";
}