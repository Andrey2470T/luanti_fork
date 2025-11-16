#pragma once

#include <mutex>
#include <memory>
#include "Core/TimeCounter.h"
#include "resource_loader.h"
#include "log.h"
#include <functional>
#include "FilesystemVersions.h"
#include "threading/mutex_auto_lock.h"
#include <Render/TTFont.h>
#include "util/numeric.h"
#include <Render/Texture2D.h>
#include "texture_modifiers.h"

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

std::vector<std::string> getTexturesDefaultPaths();
std::vector<std::string> getShaderDefaultPaths();
std::string texturePathFinder(const std::string &name);
std::string shaderPathFinder(const std::string &name);
std::string fallbackPathFinder(const std::string &name);

template <class T>
struct ResourceInfo
{
    std::string name;
    std::string path;
    std::unique_ptr<T> data;
    u32 refcount = 1;
};

class IResourceSubCache {
public:
    virtual ~IResourceSubCache() = default;
    virtual void* get(const std::string &name) = 0;
    virtual void* getByID(u32 id) = 0;
    virtual void* getOrLoad(const std::string &name) = 0;
    virtual u32 cacheResource(void* resource, const std::string &name = "") = 0;
    virtual void clearResource(u32 id, bool force=false) = 0;
    virtual void clearResource(void* resource, bool force=false) = 0;
};

template <class T>
class ResourceSubCache : public IResourceSubCache
{
    std::vector<std::unique_ptr<ResourceInfo<T>>> cache;

    std::function<std::string(const std::string&)> findPathCallback;
    std::function<T*(const std::string &)> loadCallback;
public:
    std::vector<std::string> defpaths;

    ResourceSubCache(const std::vector<std::string> &_defpaths, const std::function<std::string(std::string)> &_findPathCallback,
        const std::function<T*(const std::string &)> &_loadCallback);

    void *get(const std::string &name) override;
    void *getByID(u32 id) override;
    void *getOrLoad(const std::string &name) override;
    u32 cacheResource(void *res, const std::string &name="") override;
    void clearResource(u32 id, bool force=false) override;
    void clearResource(void *res, bool force=false) override;
};

class Atlas;

namespace render
{
    class TTFont;
}

class ResourceCache
{
    std::unordered_map<ResourceType, std::unique_ptr<IResourceSubCache>> subcaches;
    
    std::unique_ptr<ResourceLoader> loader;

    std::mutex resource_mutex;

    std::unique_ptr<TextureGenerator> texgen;
public:
    ResourceCache();

    template <class T>
    T *get(ResourceType _type, const std::string &_name);
    template <class T>
    T *getByID(ResourceType _type, u32 _id);
    template<class T>
    T *getOrLoad(ResourceType _type, const std::string &_name,
        bool apply_modifiers=false, bool load_for_mesh=false, bool apply_fallback=false);

    template<class T>
    u32 cacheResource(ResourceType _type, T *res, const std::string &name="");

    template<class T>
    void clearResource(ResourceType _type, u32 id, bool force=false);
    template<class T>
    void clearResource(ResourceType _type, T *res, bool force=false);

    img::Image *createDummyImage()
    {
        img::color8 randomColor(img::PF_RGBA8,
            myrand()%256, myrand()%256,myrand()%256
        );

        return new img::Image(img::PF_RGBA8, 1, 1, randomColor);
    }
    render::Texture2D *createDummyTexture()
    {
        auto dummyImg = createDummyImage();
        return new render::Texture2D("Unknown", std::unique_ptr<img::Image>(dummyImg));
    }
};



template<class T>
ResourceSubCache<T>::ResourceSubCache(
    const std::vector<std::string> &_defpaths, const std::function<std::string(std::string)> &_findPathCallback,
    const std::function<T*(const std::string &)> &_loadCallback)
    : findPathCallback(_findPathCallback), loadCallback(_loadCallback), defpaths(_defpaths)
{}

template<class T>
void *ResourceSubCache<T>::get(const std::string &name)
{
    auto it = std::find_if(cache.begin(), cache.end(), [name] (const std::unique_ptr<ResourceInfo<T>> &elem)
    {
        bool path1_exists = fs::exists(elem->path);
        bool path2_exists = fs::exists(name);

        bool name_cmp = elem->name == name;
        bool path_cmp = false;

        if (path1_exists && path2_exists)
            path_cmp = fs::equivalent(elem->path, name);
        return name_cmp || path_cmp;
    });

    if (it == cache.end())
        return nullptr;

    ++(it->get()->refcount);

    return it->get()->data.get();
}

template<class T>
void *ResourceSubCache<T>::getByID(u32 _id)
{
    if (_id >= cache.size()) {
        infostream << "ResourceSubCache<T>::getByID(): Resource ID " << _id << " is out of range" << std::endl;
        return nullptr;
    }

    ++(cache.at(_id).get()->refcount);

    return cache.at(_id).get()->data.get();
}

template<class T>
void *ResourceSubCache<T>::getOrLoad(const std::string &name)
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

    T *res = loadCallback(target_path);

    if (!res) {
        infostream << "ResourceSubCache<T>::getOrLoad(): Couldn't load the resource with name " << name << std::endl;
        return nullptr;
    }

    ResourceInfo<T> *info = new ResourceInfo<T>();
    info->name = name;
    info->path = target_path;
    info->data.reset(res);
    cache.emplace_back(info);
    return res;
}

template<class T>
u32 ResourceSubCache<T>::cacheResource(void *res, const std::string &name)
{
    auto it = std::find_if(cache.begin(), cache.end(), [res] (const std::unique_ptr<ResourceInfo<T>> &elem)
    {
        return elem.get()->data.get() == static_cast<T *>(res);
    });

    if (it != cache.end())
        return std::distance(cache.begin(), it);

    ResourceInfo<T> *info = new ResourceInfo<T>();
    info->name = name;
    info->path = "";
    info->data.reset(static_cast<T *>(res));
    cache.emplace_back(info);
    return cache.size()-1;
}

template<class T>
void ResourceSubCache<T>::clearResource(u32 id, bool force)
{
    if (id >= cache.size())
        return;

    auto it = cache.begin() + id;

    if (force || it->get()->refcount == 1)
        cache.erase(it);
    else
        --(it->get()->refcount);
}

template<class T>
void ResourceSubCache<T>::clearResource(void *res, bool force)
{
    auto it = std::find_if(cache.begin(), cache.end(), [res] (const std::unique_ptr<ResourceInfo<T>> &elem)
    {
        return elem.get()->data.get() == static_cast<T *>(res);
    });

    if (force || it->get()->refcount == 1)
        cache.erase(it);
    else
        --(it->get()->refcount);
}

template <class T>
T *ResourceCache::get(ResourceType _type, const std::string &_name)
{
    MutexAutoLock lock(resource_mutex);
    return static_cast<T *>(subcaches[_type]->get(_name));
}

template <class T>
T *ResourceCache::getByID(ResourceType _type, u32 _id)
{
    MutexAutoLock lock(resource_mutex);
    return static_cast<T *>(subcaches[_type]->getByID(_id));
}

template <class T>
T *ResourceCache::getOrLoad(ResourceType _type, const std::string &_name,
    bool apply_modifiers, bool load_for_mesh, bool apply_fallback)
{
    if (_name.empty())
        return nullptr;

    if (_type == ResourceType::IMAGE) {
        auto img = reinterpret_cast<img::Image *>(subcaches[_type]->get(_name));

        if (img)
            return static_cast<T *>((void*)img);
        if (!apply_modifiers) {
            MutexAutoLock lock(resource_mutex);

            img = reinterpret_cast<img::Image *>(subcaches[_type]->getOrLoad(_name));
        }
        else {
            if (load_for_mesh)
                img = texgen->generateForMesh(_name);
            else
                img = texgen->generate(_name);

            subcaches[_type]->cacheResource(img, _name);
        }

        if (!img && apply_fallback) {
            infostream << "ResourceCache::getOrLoad(): Couldn't create image \"" << _name << "\", creating a dummy 1x1 one\n";
            img = createDummyImage();
        }

        return static_cast<T *>((void*)img);
    }
    else if (_type == ResourceType::TEXTURE) {
        MutexAutoLock lock(resource_mutex);
        auto tex = reinterpret_cast<render::Texture2D *>(subcaches[_type]->getOrLoad(_name));

        if (!tex && apply_fallback)
            tex = createDummyTexture();
        return static_cast<T *>((void*)tex);
    }
    else {
        MutexAutoLock lock(resource_mutex);
        return static_cast<T *>(subcaches[_type]->getOrLoad(_name));
    }
}

template<class T>
u32 ResourceCache::cacheResource(ResourceType _type, T *res, const std::string &name)
{
    if (!res)
        return 0;
    MutexAutoLock lock(resource_mutex);
    return subcaches[_type]->cacheResource(res, name);
}

template<class T>
void ResourceCache::clearResource(ResourceType _type, u32 id, bool force)
{
    MutexAutoLock lock(resource_mutex);
    subcaches[_type]->clearResource(id, force);
}

template<class T>
void ResourceCache::clearResource(ResourceType _type, T *res, bool force)
{
    if (!res)
        return;
    MutexAutoLock lock(resource_mutex);
    subcaches[_type]->clearResource(res, force);
}
