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

    std::list<LayeredMesh *> meshes;

    std::mutex meshes_mutex;

    std::list<BatchedLayer> layers;

    std::mutex drawlist_mutex;

    std::unique_ptr<DrawListUpdateThread> drawlist_thread;

    std::atomic<bool> needs_update_drawlist;

    bool cache_trilinear_filter;
    bool cache_bilinear_filter;
    bool cache_anistropic_filter;
    u16 cache_transparency_sorting_distance;
public:
    DistanceSortedDrawList(Client *_client, DrawControl _draw_control);

    ~DistanceSortedDrawList();

    void addLayeredMesh(LayeredMesh *newMesh);
    void removeLayeredMesh(LayeredMesh *mesh);

    class LayeredMeshSorter
    {
    public:
        v3f camera_pos;

        LayeredMeshSorter() = default;

        bool operator() (const LayeredMesh *m1, const LayeredMesh *m2) const;
    };

    LayeredMeshSorter mesh_sorter;
    DrawControl draw_control;

    const DrawControl &getDrawControl() const
    {
        return draw_control;
    }

    void lockMeshes()
    {
        meshes_mutex.lock();
    }

    void unlockMeshes()
    {
        meshes_mutex.unlock();
    }
    
    void forceUpdate()
    {
    	needs_update_drawlist = true;
    }

    void updateList();

    void render();

    void onSettingChanged(std::string_view name, bool all);
private:
    bool isMeshOccluded(LayeredMesh *mesh, u16 mesh_size);
};
