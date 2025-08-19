#include "drawlist.h"
#include "client/client.h"
#include "client/mesh/layeredmesh.h"
#include "client/player/playercamera.h"
#include "client/render/tilelayer.h"
#include "settings.h"
#include "util/numeric.h"
#include "renderer.h"
#include "rendersystem.h"

static void on_settings_changed(const std::string &name, void *data)
{
    static_cast<DistanceSortedDrawList*>(data)->onSettingChanged(name, false);
}

static const std::string ClientMap_settings[] = {
    "trilinear_filter",
    "bilinear_filter",
    "anisotropic_filter",
    "transparency_sorting_distance",
    "enable_dynamic_shadows",
    "enable_translucent_foliage"
};

DistanceSortedDrawList::DistanceSortedDrawList(Client *_client, DrawControl _draw_control)
    : client(_client), draw_control(_draw_control)
{
    for (const auto &name : ClientMap_settings)
        g_settings->registerChangedCallback(name, on_settings_changed, this);
    // load all settings at once
    onSettingChanged("", true);

    drawlist_thread = std::make_unique<DrawListUpdateThread>(this);
    drawlist_thread->start();
}

DistanceSortedDrawList::~DistanceSortedDrawList()
{
    g_settings->deregisterAllChangedCallbacks(this);

    drawlist_thread->stop();
    drawlist_thread->wait();
}

// The meshes_mutex must be locked externally before this call!
void DistanceSortedDrawList::addLayeredMesh(LayeredMesh *newMesh, bool shadow)
{
    auto &list = shadow ? shadow_meshes : meshes;
    auto find_mesh = std::find(list.begin(), list.end(), newMesh);

    if (find_mesh != list.end())
        return;

    list.push_back(newMesh);

    if (!shadow)
        needs_update_drawlist = true;
    else
        needs_update_shadow_drawlist = true;
}

// The meshes_mutex must be locked externally before this call!
void DistanceSortedDrawList::removeLayeredMesh(LayeredMesh *mesh, bool shadow)
{
    auto &list = shadow ? shadow_meshes : meshes;
    list.remove(mesh);

    if (!shadow)
        needs_update_drawlist = true;
    else
        needs_update_shadow_drawlist = true;
}

bool DistanceSortedDrawList::LayeredMeshSorter::operator() (const LayeredMesh *m1, const LayeredMesh *m2) const
{
    v3f m1_center = m1->getBoundingSphereCenter();
    v3f m2_center = m2->getBoundingSphereCenter();

    auto dist1 = m1_center.getDistanceFromSQ(camera_pos);
    auto dist2 = m2_center.getDistanceFromSQ(camera_pos);
    return dist1 > dist2 || (dist1 == dist2 && m1_center > m2_center);
}

void DistanceSortedDrawList::onSettingChanged(std::string_view name, bool all)
{
    if (all || name == "trilinear_filter")
        cache_trilinear_filter  = g_settings->getBool("trilinear_filter");
    if (all || name == "bilinear_filter")
        cache_bilinear_filter   = g_settings->getBool("bilinear_filter");
    if (all || name == "anisotropic_filter")
        cache_anistropic_filter = g_settings->getBool("anisotropic_filter");
    if (all || name == "transparency_sorting_distance")
        cache_transparency_sorting_distance = g_settings->getU16("transparency_sorting_distance");
    if (all || name == "enable_dynamic_shadows")
        enable_shadows = g_settings->getBool("enable_dynamic_shadows");
    if (all || name == "enable_translucent_foliage")
        translucent_foliage = g_settings->getBool("enable_translucent_foliage");
}

void DistanceSortedDrawList::updateList()
{
    if (!needs_update_drawlist)
        return;

    needs_update_drawlist = false;

    std::list<LayeredMesh *> new_meshes_list;

    MutexAutoLock meshes_lock(meshes_mutex);

    //MeshGrid mesh_grid = client->getMeshGrid();

    v3f cameraPos = client->getCamera()->getPosition();

    for (auto &mesh : meshes) {
        v3f center = mesh->getBoundingSphereCenter();
        f32 radius = mesh->getBoundingSphereRadius();

        // Check that the mesh distance doesn't exceed the wanted range
        if (!draw_control.range_all &&
            center.getDistanceFrom(cameraPos) >
                draw_control.wanted_range * BS + radius)
            continue;

        // Frustum culling
        // Only do coarse culling here, to account for fast camera movement.
        // This is needed because this function is not called every frame.
        f32 frustum_cull_extra_radius = 300.0f;
        if (mesh->isFrustumCulled(client->getCamera(), frustum_cull_extra_radius)) {
            continue;
        }

        // NOTE: works only for mapblocks now
        // Raytraced occlusion culling - send rays from the camera to the block's corners
        /*if (!draw_control.range_all && mesh_grid.cell_size < 4 &&
                isMeshOccluded(mesh, mesh_grid.cell_size)) {
            continue;
        }*/

        new_meshes_list.push_back(mesh);
    }

    meshes_mutex.unlock();

    // Resort the new mesh list
    mesh_sorter.camera_pos = cameraPos;
    std::sort(new_meshes_list.begin(), new_meshes_list.end(), mesh_sorter);

    MutexAutoLock drawlist_lock(drawlist_mutex);

    f32 sorting_distance = cache_transparency_sorting_distance * BS;

    layers.clear();

    // At first add solid layers, then transparent
    for (auto &mesh : new_meshes_list) {
        auto all_layers = mesh->getAllLayers();

        for (auto &layer : all_layers) {
            if (layer.first->material_flags & MATERIAL_FLAG_TRANSPARENT)
                continue;
                
            auto find_layer = std::find(layers.begin(), layers.end(), layer.first.get());
            
            if (find_layer == layers.end()) {
                layers.emplace_back(layer.first, {});
                find_layer = std::prev(layers.end());
            }
            find_layer->second.emplace_back(layer.second, mesh);
        }

        v3f center = mesh->getBoundingSphereCenter();
        f32 radius = mesh->getBoundingSphereRadius();
        f32 distance_sq = cameraPos.getDistanceFromSQ(center);

        if (distance_sq <= std::pow(sorting_distance + radius, 2.0f)) {
            mesh->transparentSort(cameraPos);
            mesh->updateIndexBuffers();

            auto partial_layers = mesh->getPartialLayers();

            for (auto &partial_layer : partial_layers) {
            	auto buf_layer = mesh->getBufferLayer(partial_layer.buffer_id, partial_layer.layer_id);
                layers.emplace_back(buf_layer.first, {partial_layer, mesh});
            }
        }
    }
}

void DistanceSortedDrawList::resortShadowList()
{
    if (!needs_update_shadow_drawlist)
        return;

    needs_update_shadow_drawlist = false;

    MutexAutoLock list_lock(shadow_meshes_mutex);

    mesh_sorter.camera_pos = cur_light_pos;
    std::sort(shadow_meshes.begin(), shadow_meshes.end(), mesh_sorter);
}

void DistanceSortedDrawList::render()
{
    auto rndsys = client->getRenderSystem();
    auto rnd = rndsys->getRenderer();
    auto ctxt = rnd->getContext();

    if (draw_control.show_wireframe)
        ctxt->setPolygonMode(render::CM_FRONT_AND_BACK, render::PM_LINE);

    v3s16 cameraOffset = client->getCamera()->getOffset();

    MutexAutoLock drawlist_lock(drawlist_mutex);
    for (auto &l : layers) {
        l.first->setupRenderState(rndsys);

        for (auto &mesh_l : l.second) {
            matrix4 t;
            v3f center = mesh_l.second->getBoundingSphereCenter();
            t.setTranslation(center - intToFloat(cameraOffset, BS));
            t.setRotationDegrees(mesh_l.second->getRotation());
            rnd->setTransformMatrix(TMatrix::World, t);

            auto &lp = mesh_l.first;

            rnd->draw(mesh_l.second->getBuffer(lp.buffer_id), render::PT_TRIANGLES, lp.offset, lp.count);
        }
    }
}

void DistanceSortedDrawList::renderShadows(const TileLayer &override_layer)
{
    auto rndsys = client->getRenderSystem();
    auto rnd = rndsys->getRenderer();
    auto ctxt = rnd->getContext();

    v3s16 cameraOffset = client->getCamera()->getOffset();

    MutexAutoLock list_lock(shadow_meshes_mutex);
    for (auto &m : shadow_meshes) {
        matrix4 t;
        v3f center = m->getBoundingSphereCenter();
        t.setTranslation(center - intToFloat(cameraOffset, BS));
        rnd->setTransformMatrix(TMatrix::World, t);

        for (auto &l : m->getAllLayers()) {
            override_layer.material_type = l.first->material_type;
            override_layer.setupRenderState(rndsys);

            if (translucent_foliage && l.first->material_type & TILE_MATERIAL_WAVING_LEAVES)
                ctxt->setCullMode(render::CM_FRONT);

            rnd->draw(m->getBuffer(l.second.buffer_id), render::PT_TRIANGLES, l.second.offset, l.second.count);
        }
    }
}

void *DrawListUpdateThread::run()
{
    BEGIN_DEBUG_EXCEPTION_HANDLER

    while (!stopRequested()) {
        drawlist->resortShadowList();
        drawlist->updateList();
        sleep_ms(50);
    }

    END_DEBUG_EXCEPTION_HANDLER
}

bool DistanceSortedDrawList::isMeshOccluded(LayeredMesh *mesh, u16 mesh_size)
{
}
