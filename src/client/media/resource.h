#pragma once

#include "BasicIncludes.h"
#include <optional>
#include <mutex>

namespace img
{
    class Image;
};

namespace render
{
    class Texture2D;
    class Shader;
};

class MeshBuffer;
class ResourceLoader;

class enum ResourceType
{
    TEXTURE,
    IMAGE,
    SHADER,
    MESH
};

struct ResourceInfo
{
    ResourceType type;
    std::string name;

    std::string path;
};

struct ImageResourceInfo : public ResourceInfo
{
    std::unique_ptr<img::Image> data;

    static std::vector<std::string> defpaths;
}

struct TextureResourceInfo : public ResourceInfo
{
    std::unique_ptr<render::Texture2D> data;

    static std::vector<std::string> defpaths;
}

struct ShaderResourceInfo : public ResourceInfo
{
    std::unique_ptr<render::Shader> data;

    static std::vector<std::string> defpaths;
}

struct MeshResourceInfo : public ResourceInfo
{
    std::unique_ptr<MeshBuffer> data;
}

class ResourceCache
{
    std::vector<std::unique_ptr<ImageResourceInfo>> images;
    std::vector<std::unique_ptr<TextureResourceInfo>> textures;
    std::vector<std::unique_ptr<ShaderResourceInfo>> shaders;
    std::vector<std::unique_ptr<MeshResourceInfo>> meshes;
    
    ResourceLoader loader;

    std::mutex resource_mutex;
public:
    ResourceCache() = default;
    ResourceInfo *get(ResourceType _type, const std::string &_name) const;
    ResourceInfo *getByID(ResourceType _type, u32 _id) const;

    ResourceInfo *getOrLoad(ResourceType _type, const std::string &_name);

private:
    ResourceInfo *getResourceByID(const std::vector<std::unique_ptr<ResourceInfo>> &resources, u32 id) const;
    std::string findFirstExistentDefaultPath(const std::vector<std::string> &defpaths, const std::string &name, ResourceType type);
};