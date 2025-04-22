#pragma once

#include <mutex>
#include <memory>
#include "resource_loader.h"

enum class ResourceType
{
    TEXTURE,
    IMAGE,
    SHADER,
    MESH,
    PALETTE
};

struct ResourceInfo
{
    virtual ~ResourceInfo() = default;

    ResourceType type;
    std::string name;

    std::string path;
};

struct ImageResourceInfo : public ResourceInfo
{
    std::unique_ptr<img::Image> data;

    static std::vector<std::string> defpaths;
};

struct TextureResourceInfo : public ResourceInfo
{
    std::unique_ptr<render::Texture2D> data;

    static std::vector<std::string> defpaths;
};

struct ShaderResourceInfo : public ResourceInfo
{
    std::unique_ptr<render::Shader> data;

    static std::vector<std::string> defpaths;
};

struct MeshResourceInfo : public ResourceInfo
{
    std::unique_ptr<MeshBuffer> data;
};

struct PaletteResourceInfo : public ResourceInfo
{
    std::unique_ptr<img::Palette> data;
};

class ResourceCache
{
    std::vector<std::unique_ptr<ImageResourceInfo>> images;
    std::vector<std::unique_ptr<TextureResourceInfo>> textures;
    std::vector<std::unique_ptr<ShaderResourceInfo>> shaders;
    std::vector<std::unique_ptr<MeshResourceInfo>> meshes;
    std::vector<std::unique_ptr<PaletteResourceInfo>> palettes;
    
    std::unique_ptr<ResourceLoader> loader;

    std::mutex resource_mutex;
public:
    ResourceCache(img::ImageModifier *mdf);

    ResourceInfo *get(ResourceType _type, const std::string &_name);
    ResourceInfo *getByID(ResourceType _type, u32 _id);

    ResourceInfo *getOrLoad(ResourceType _type, const std::string &_name);

private:
    std::string findFirstExistentDefaultPath(const std::vector<std::string> &defpaths, const std::string &name, ResourceType type);
};
