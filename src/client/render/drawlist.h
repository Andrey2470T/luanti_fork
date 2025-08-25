#pragma once

#include "threading/thread.h"
#include "debug.h"
#include "porting.h"
#include <memory>
#include <list>
#include <atomic>

class Client;
class LayeredMesh;
struct TileLayer;
struct LayeredMeshPart;
class SelectionMesh;
class BlockBounds;
class Camera;

typedef std::pair<std::shared_ptr<TileLayer>,
    std::vector<std::pair<LayeredMeshPart, LayeredMesh*>>> BatchedLayer;

class DistanceSortedDrawList;

class DrawListUpdateThread : public Thread
{
public:
    DrawListUpdateThread(DistanceSortedDrawList *list)
        : Thread("DrawList"), drawlist(list)
    {}

    void *run();
private:
    DistanceSortedDrawList *drawlist;
};

struct DrawControl
{
    // Wanted drawing range
    f32 wanted_range = 0.0f;
    // Overrides limits by drawing everything
    bool range_all = false;
    // Allow rendering out of bounds
    bool allow_noclip = false;
    // show a wire frame for debugging
    bool show_wireframe = false;
};

// Sorted by distance list of layers batched from each layered mesh
// Allows sorting different objects (mapblocks, objects) together in the single list in order to avoid transparency issues
class DistanceSortedDrawList
{
    Client *client;
    Camera *camera;

    std::list<LayeredMesh *> meshes;
    std::list<LayeredMesh *> shadow_meshes;

    std::mutex meshes_mutex;
    std::mutex shadow_meshes_mutex;

    std::list<BatchedLayer> layers;

    std::mutex drawlist_mutex;

    std::unique_ptr<DrawListUpdateThread> drawlist_thread;

    std::atomic<bool> needs_update_drawlist;
    std::atomic<bool> needs_update_shadow_drawlist;

    v3f cur_light_pos;

    std::unique_ptr<SelectionMesh> selection_mesh;
    std::unique_ptr<BlockBounds> block_bounds;

    bool cache_trilinear_filter;
    bool cache_bilinear_filter;
    bool cache_anistropic_filter;
    u16 cache_transparency_sorting_distance;
    bool enable_shadows;
    bool translucent_foliage;
public:
    DistanceSortedDrawList(Client *_client);

    ~DistanceSortedDrawList();

    void addLayeredMesh(LayeredMesh *newMesh, bool shadow=false);
    void removeLayeredMesh(LayeredMesh *mesh, bool shadow=false);

    class LayeredMeshSorter
    {
    public:
        v3f camera_pos;

        LayeredMeshSorter() = default;

        bool operator() (const LayeredMesh *m1, const LayeredMesh *m2) const;
    };

    LayeredMeshSorter mesh_sorter;
    DrawControl draw_control;

    DrawControl &getDrawControl()
    {
        return draw_control;
    }

    SelectionMesh *getSelectionMesh() const
    {
        return selection_mesh.get();
    }
    BlockBounds *getBlockBounds() const
    {
        return block_bounds.get();
    }

    void lockMeshes(bool shadow=false)
    {
        if (!shadow)
            meshes_mutex.lock();
        else
            shadow_meshes_mutex.lock();
    }

    void unlockMeshes(bool shadow=false)
    {
        if (!shadow)
            meshes_mutex.unlock();
        else
            shadow_meshes_mutex.unlock();
    }
    
    void forceUpdate()
    {
    	needs_update_drawlist = true;
    }

    void setLightPos(const v3f &new_pos)
    {
        cur_light_pos = new_pos;
    }

    v3f getLightPos() const
    {
        return cur_light_pos;
    }

    void updateList();
    void resortShadowList();

    void render();
    void renderShadows(TileLayer &override_layer);

    void onSettingChanged(std::string_view name, bool all);
private:
    bool isMeshOccluded(LayeredMesh *mesh, u16 mesh_size);
};
