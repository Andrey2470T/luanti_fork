#pragma once

#include "threading/thread.h"
#include "debug.h"
#include "porting.h"
#include <memory>
#include <list>
#include <atomic>
#include <set>
#include <shared_mutex>

class Client;
class LayeredMesh;
struct TileLayer;
struct LayeredMeshPart;
class SelectionMesh;
class BlockBounds;
class Camera;

typedef std::vector<std::pair<LayeredMeshPart, LayeredMesh *>> BatchedLayerParts;
typedef std::pair<TileLayer, BatchedLayerParts> BatchedLayer;

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

#define FOG_RANGE_ALL (100000 * BS)

struct DrawControl
{
    // Wanted drawing range
    f32 wanted_range = 0.0f;
    f32 fog_range = 0.0f;
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

    std::unordered_map<u32, LayeredMesh *> meshes;
    std::unordered_map<u32, LayeredMesh *> shadow_meshes;
    u32 meshes_free_id{0};

    // Blocks meshes and visible_meshes lists
    std::shared_mutex meshes_mutex;
    // Block shadow_meshes list
    std::shared_mutex shadow_meshes_mutex;

    std::list<LayeredMesh *> visible_meshes;

    std::unordered_map<LayeredMesh *, bool> back_meshes_queue;
    std::unordered_map<LayeredMesh *, bool> front_meshes_queue;
    std::shared_mutex queue_mutex;
    
    std::list<BatchedLayer> layers;
    std::shared_mutex layers_mutex;

    std::unique_ptr<DrawListUpdateThread> drawlist_thread;

    std::atomic<bool> needs_update_drawlist;
    std::atomic<bool> needs_update_shadow_drawlist;
    std::atomic<bool> needs_upload_indices;

    v3f cur_light_pos;

    std::unique_ptr<SelectionMesh> selection_mesh;
    std::unique_ptr<BlockBounds> block_bounds;

    class LayeredMeshSorter
    {
    public:
        v3f camera_pos;

        LayeredMeshSorter() = default;

        bool operator() (const LayeredMesh *m1, const LayeredMesh *m2) const;
    };

    LayeredMeshSorter mesh_sorter;
    DrawControl draw_control;

    bool cache_trilinear_filter;
    bool cache_bilinear_filter;
    bool cache_anistropic_filter;
    u16 cache_transparency_sorting_distance;
    bool enable_shadows;
    bool translucent_foliage;
public:
    DistanceSortedDrawList(Client *_client);

    ~DistanceSortedDrawList();

    void addLayeredMeshes(const std::set<LayeredMesh *> &newMeshes, bool shadow=false);
    void removeLayeredMeshes(const std::set<LayeredMesh *> &meshes, bool shadow=false);

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
    //void resortShadowList();

    void render();
    //void renderShadows(TileLayer &override_layer);

    void onSettingChanged(std::string_view name, bool all);
private:
    bool isMeshOccluded(LayeredMesh *mesh, u16 mesh_size);
};
