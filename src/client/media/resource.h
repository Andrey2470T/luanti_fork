#pragma once

#include <mutex>
#include <memory>
#include "resource_loader.h"
#include "log.h"
#include <functional>
#include "FilesystemVersions.h"
#include "threading/mutex_auto_lock.h"
#include <Render/TTFont.h>

enum class ResourceType
{
    TEXTURE,
    IMAGE,
    SHADER,
    MODEL,
    PALETTE,
    ATLAS,
    FONT
};


template <class T>
struct ResourceInfo
{
    std::string name;
    std::string path;
    std::unique_ptr<T> data;
};

template <class T>
class ResourceSubCache
{
    std::vector<std::unique_ptr<ResourceInfo<T>>> cache;

    std::function<std::string(const std::string&)> findPathCallback;
    std::function<T*(const std::string &)> loadCallback;
public:
    std::vector<std::string> defpaths;

    ResourceSubCache(const std::vector<std::string> &_defpaths, const std::function<std::string(std::string)> &_findPathCallback,
        const std::function<T*(const std::string &)> &_loadCallback);

    T *get(const std::string &name);
    T *getByID(u32 id);
    T *getOrLoad(const std::string &name);
    u32 cacheResource(T *res, const std::string &name="");
    void clearResource(u32 id);
    void clearResource(T *res);
};

class Atlas;

namespace render
{
    class TTFont;
}

class ResourceCache
{
    std::unique_ptr<ResourceSubCache<img::Image>> images;
    std::unique_ptr<ResourceSubCache<render::Texture2D>> textures;
    std::unique_ptr<ResourceSubCache<render::Shader>> shaders;
    std::unique_ptr<ResourceSubCache<Model>> models;
    std::unique_ptr<ResourceSubCache<img::Palette>> palettes;
    std::unique_ptr<ResourceSubCache<Atlas>> atlases;
    std::unique_ptr<ResourceSubCache<render::TTFont>> fonts;
    
    std::unique_ptr<ResourceLoader> loader;

    std::mutex resource_mutex;
public:
    ResourceCache(main::OpenGLVersion version);

    template <class T>
    T *get(ResourceType _type, const std::string &_name);
    template <class T>
    T *getByID(ResourceType _type, u32 _id);
    template<class T>
    T *getOrLoad(ResourceType _type, const std::string &_name);

    template<class T>
    u32 cacheResource(ResourceType _type, T *res, const std::string &name="");

    template<class T>
    void clearResource(ResourceType _type, u32 id);
    template<class T>
    void clearResource(ResourceType _type, T *res);
};



template<class T>
ResourceSubCache<T>::ResourceSubCache(
    const std::vector<std::string> &_defpaths, const std::function<std::string(std::string)> &_findPathCallback,
    const std::function<T*(const std::string &)> &_loadCallback)
    : defpaths(_defpaths), findPathCallback(_findPathCallback), loadCallback(_loadCallback)
{}

template<class T>
T *ResourceSubCache<T>::get(const std::string &name)
{
    auto it = std::find(cache.begin(), cache.end(), [name] (const std::unique_ptr<ResourceInfo<T>> &elem)
    {
        return elem.name == name;
    });

    if (it == cache.end())
        return nullptr;

    return it->get()->data.get();
}

template<class T>
T *ResourceSubCache<T>::getByID(u32 _id)
{
    if (_id > cache.size()-1) {
        infostream << "ResourceSubCache<T>::getByID(): Resource ID " << _id << " is out of range" << std::endl;
        return nullptr;
    }

    return cache.at(_id).get()->data.get();
}

template<class T>
T *ResourceSubCache<T>::getOrLoad(const std::string &name)
{
    auto *cachedRes = get(name);

    if (cachedRes)
        return cachedRes;

    if (!findPathCallback) {
    	infostream << "ResourceSubCache<T>::getOrLoad(): No finding path callback provided" << std::endl;
        return nullptr;
    }
    std::string target_path = findPathCallback(name);

    if (target_path.empty()) {
        fs::path abs_path = fs::absolute(name);

        if (fs::exists(abs_path))
            target_path = abs_path.parent_path().string();
        else {
            infostream << "ResourceSubCache<T>::getOrLoad(): Couldn't find any path to the resource with name " << name << std::endl;
            return nullptr;
        }
    }

    if (!loadCallback) {
    	infostream << "ResourceSubCache<T>::getOrLoad(): No loading callback provided" << std::endl;
        return nullptr;
    }
    T *res = loadCallback(name);

    if (!res) {
        infostream << "ResourceSubCache<T>::getOrLoad(): Couldn't load the resource with name " << name << std::endl;
        return nullptr;
    }

    cache.emplace_back(name, target_path, res);
    return res;
}

template<class T>
u32 ResourceSubCache<T>::cacheResource(T *res, const std::string &name)
{
    auto it = std::find(cache.begin(), cache.end(), [res] (const std::unique_ptr<ResourceInfo<T>> &elem)
    {
        return elem.get()->data.get() == res;
    });

    if (it != cache.end())
        return std::distance(cache.begin(), it);

    cache.emplace_back(name, "", res);
    return cache.size()-1;
}

template<class T>
void ResourceSubCache<T>::clearResource(u32 id)
{
    if (id > cache.size()-1)
        return;

    cache.erase(cache.begin() + id);
}

template<class T>
void ResourceSubCache<T>::clearResource(T *res)
{
    auto it = std::find(cache.begin(), cache.end(), [res] (const std::unique_ptr<ResourceInfo<T>> &elem)
    {
        return elem.get()->data.get() == res;
    });

    if (it != cache.end())
        cache.erase(it);
}

template <class T>
T *ResourceCache::get(ResourceType _type, const std::string &_name)
{
    MutexAutoLock lock(resource_mutex);
    switch (_type) {
    case ResourceType::IMAGE:
        return images->get(_name);
    case ResourceType::TEXTURE:
        return textures->get(_name);
    case ResourceType::SHADER:
        return shaders->get(_name);
    case ResourceType::MODEL:
        return models->get(_name);
    case ResourceType::PALETTE:
        return palettes->get(_name);
    case ResourceType::ATLAS:
        return atlases->get(_name);
    case ResourceType::FONT:
        return fonts->get(_name);
    }
}

template <class T>
T *ResourceCache::getByID(ResourceType _type, u32 _id)
{
    MutexAutoLock lock(resource_mutex);
    switch (_type) {
    case ResourceType::IMAGE:
        return images->getByID(_id);
    case ResourceType::TEXTURE:
        return textures->getByID(_id);
    case ResourceType::SHADER:
        return shaders->getByID(_id);
    case ResourceType::MODEL:
        return models->getByID(_id);
    case ResourceType::PALETTE:
        return palettes->getByID(_id);
    case ResourceType::ATLAS:
        return atlases->getByID(_id);
    case ResourceType::FONT:
        return fonts->getByID(_id);
    }
}

template <class T>
T *ResourceCache::getOrLoad(ResourceType _type, const std::string &_name)
{
    MutexAutoLock lock(resource_mutex);
    switch (_type) {
    case ResourceType::IMAGE:
        return images->getOrLoad(_name);
    case ResourceType::TEXTURE:
        return textures->getOrLoad(_name);
    case ResourceType::SHADER:
        return shaders->getOrLoad(_name);
    case ResourceType::MODEL:
        return models->getOrLoad(_name);
    case ResourceType::PALETTE:
        return palettes->getOrLoad(_name);
    case ResourceType::ATLAS:
        return atlases->getOrLoad(_name);
    case ResourceType::FONT:
        return fonts->getOrLoad(_name);
    }
}

template<class T>
u32 ResourceCache::cacheResource(ResourceType _type, T *res, const std::string &name)
{
    MutexAutoLock lock(resource_mutex);
    switch (_type) {
    case ResourceType::IMAGE:
        return images->cacheResource(res, name);
    case ResourceType::TEXTURE:
        return textures->cacheResource(res, name);
    case ResourceType::SHADER:
        return shaders->cacheResource(res, name);
    case ResourceType::MODEL:
        return models->cacheResource(res, name);
    case ResourceType::PALETTE:
        return palettes->cacheResource(res, name);
    case ResourceType::ATLAS:
        return atlases->cacheResource(res, name);
    case ResourceType::FONT:
        return fonts->cacheResource(res, name);
    }
}

template<class T>
void ResourceCache::clearResource(ResourceType _type, u32 id)
{
    MutexAutoLock lock(resource_mutex);
    switch (_type) {
    case ResourceType::IMAGE:
        return images->clearResource(id);
    case ResourceType::TEXTURE:
        return textures->clearResource(id);
    case ResourceType::SHADER:
        return shaders->clearResource(id);
    case ResourceType::MODEL:
        return models->clearResource(id);
    case ResourceType::PALETTE:
        return palettes->clearResource(id);
    case ResourceType::ATLAS:
        return atlases->clearResource(id);
    case ResourceType::FONT:
        return fonts->clearResource(id);
    }
}

template<class T>
void ResourceCache::clearResource(ResourceType _type, T *res)
{
    MutexAutoLock lock(resource_mutex);
    switch (_type) {
    case ResourceType::IMAGE:
        return images->clearResource(res);
    case ResourceType::TEXTURE:
        return textures->clearResource(res);
    case ResourceType::SHADER:
        return shaders->clearResource(res);
    case ResourceType::MODEL:
        return models->clearResource(res);
    case ResourceType::PALETTE:
        return palettes->clearResource(res);
    case ResourceType::ATLAS:
        return atlases->clearResource(res);
    case ResourceType::FONT:
        return fonts->clearResource(res);
    }
}
