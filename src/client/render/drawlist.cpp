#include "drawlist.h"
#include "client/core/client.h"
#include "client/mesh/layeredmesh.h"
#include "client/player/playercamera.h"
#include "client/render/tilelayer.h"
#include "settings.h"
#include "util/numeric.h"
#include "renderer.h"
#include "rendersystem.h"
#include "client/player/selection.h"
#include <shared_mutex>

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

void LayeredMeshRef::deleteIfMarked()
{
    if (marked_for_deletion) {
        delete ptr;
        ptr = nullptr;
    }
}

DistanceSortedDrawList::DistanceSortedDrawList(Client *_client)
    : client(_client), camera(client->getEnv().getLocalPlayer()->getCamera())
{
    mesh_sorter.meshes_ref = &meshes;

    for (const auto &name : ClientMap_settings)
        g_settings->registerChangedCallback(name, on_settings_changed, this);
    // load all settings at once
    onSettingChanged("", true);

    selection_mesh = std::make_unique<SelectionMesh>(client->getRenderSystem(), client->getResourceCache());
    block_bounds = std::make_unique<BlockBounds>(client->getRenderSystem());
    drawlist_thread = std::make_unique<DrawListUpdateThread>(this);
    drawlist_thread->start();
}

DistanceSortedDrawList::~DistanceSortedDrawList()
{
    g_settings->deregisterAllChangedCallbacks(this);

    drawlist_thread->stop();
}

s32 DistanceSortedDrawList::addLayeredMesh(LayeredMesh *newMesh, bool shadow)
{
    if (!newMesh || newMesh->getBuffersCount() == 0) return -1;

    std::unique_lock meshes_lock(meshes_mutex);

    auto &list = shadow ? shadow_meshes : meshes;

    u32 mesh_id = meshes_free_id;
    list[mesh_id] = newMesh;

    while (list.find(meshes_free_id) != list.end())
        meshes_free_id++;

    if (!shadow)
        needs_update_drawlist = true;
    else
        needs_update_shadow_drawlist = true;

    return mesh_id;
}

void DistanceSortedDrawList::removeLayeredMesh(u32 meshId, LayeredMesh *mesh, bool shadow)
{
    std::unique_lock delete_lock(delete_mutex);

    pending_to_delete_meshes[meshId] = mesh;

    if (!shadow)
        needs_update_drawlist = true;
    else
        needs_update_shadow_drawlist = true;
}

bool DistanceSortedDrawList::LayeredMeshSorter::operator() (const u32 m1_n, const u32 m2_n) const
{
    auto m1 = (*meshes_ref)[m1_n];
    auto m2 = (*meshes_ref)[m2_n];

    v3f m1_center = m1 ? m1->getBoundingSphereCenter() : v3f();
    v3f m2_center = m2 ? m2->getBoundingSphereCenter() : v3f();

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

    v3f cameraPos = camera->getPosition();

    std::unordered_map<u32, LayeredMesh *> local_meshes;
    std::list<u32> local_visible_meshes;

    {
        std::shared_lock delete_lock(delete_mutex);
        std::unique_lock meshes_lock(meshes_mutex);

        for (auto &mesh_p : pending_to_delete_meshes) {
            meshes_free_id = std::min(meshes_free_id, mesh_p.first);
            meshes.erase(mesh_p.first);
        }

        local_meshes = meshes;
    }

    //MeshGrid mesh_grid = client->getMeshGrid();

    for (auto &mesh_p : local_meshes) {
        auto mesh = mesh_p.second;
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
        if (mesh->isFrustumCulled(camera, frustum_cull_extra_radius))
            continue;

        // NOTE: works only for mapblocks now
        // Raytraced occlusion culling - send rays from the camera to the block's corners
        /*if (!draw_control.range_all && mesh_grid.cell_size < 4 &&
                isMeshOccluded(mesh, mesh_grid.cell_size)) {
            continue;
        }*/

        local_visible_meshes.push_back(mesh_p.first);
    }

    // Resort the new mesh list
    mesh_sorter.camera_pos = cameraPos;
    local_visible_meshes.sort(mesh_sorter);

    f32 sorting_distance = cache_transparency_sorting_distance * BS;

    std::list<BatchedLayer> newlayers;

    // At first add solid layers, then transparent
    for (auto &mesh_n : local_visible_meshes) {
        auto mesh = local_meshes[mesh_n];
        auto all_layers = mesh->getAllLayers();

        for (auto &layer : all_layers) {
            if (layer.first.material_flags & MATERIAL_FLAG_TRANSPARENT)
                continue;

            auto find_layer = std::find_if(newlayers.begin(), newlayers.end(),
                [layer] (const BatchedLayer &cur_layer)
                {
                    return cur_layer.first == layer.first;
            });
            
            if (find_layer == newlayers.end()) {
                newlayers.emplace_back(layer.first, BatchedLayerParts());
                find_layer = std::prev(newlayers.end());
            }
            find_layer->second.emplace_back(layer.second, mesh_n);
        }

        v3f center = mesh->getBoundingSphereCenter();
        f32 radius = mesh->getBoundingSphereRadius();
        f32 distance_sq = cameraPos.getDistanceFromSQ(center);

        if (distance_sq <= std::pow(sorting_distance + radius, 2.0f)) {
            mesh->transparentSort(cameraPos);
            needs_upload_indices = mesh->updateIndexBuffers();

            auto partial_layers = mesh->getPartialLayers();

            for (auto &partial_layer : partial_layers) {
                BatchedLayerParts mesh_parts;
                mesh_parts.emplace_back(partial_layer, mesh_n);
                newlayers.emplace_back(partial_layer.layer, mesh_parts);
            }
        }
    }

    {
        std::unique_lock meshes_lock(meshes_mutex);
        visible_meshes = local_visible_meshes;
    }
    {
        std::unique_lock drawlist_lock(drawlist_mutex);
        layers = newlayers;
    }
}

/*void DistanceSortedDrawList::resortShadowList()
{
    if (!needs_update_shadow_drawlist)
        return;

    needs_update_shadow_drawlist = false;

    MutexAutoLock list_lock(shadow_meshes_mutex);

    mesh_sorter.camera_pos = cur_light_pos;
    shadow_meshes.sort(mesh_sorter);
}*/

void DistanceSortedDrawList::render()
{
    auto rndsys = client->getRenderSystem();
    auto rnd = rndsys->getRenderer();
    auto ctxt = rnd->getContext();

    if (draw_control.show_wireframe)
        ctxt->setPolygonMode(render::CM_FRONT_AND_BACK, render::PM_LINE);

    v3s16 cameraOffset = camera->getOffset();

    if (!pending_to_delete_meshes.empty()) {
        std::unique_lock delete_lock(delete_mutex);

        for (auto &mesh_p : pending_to_delete_meshes)
            delete mesh_p.second;
        pending_to_delete_meshes.clear();
    }
    if (needs_upload_indices) {
        std::shared_lock meshes_lock(meshes_mutex);

        needs_upload_indices = false;

        for (auto &mesh_n : visible_meshes) {
            auto mesh = meshes[mesh_n];

            if (!mesh)
                continue;
            // upload new indices lists for each buffer
            for (u8 buf_i = 0; buf_i < mesh->getBuffersCount(); buf_i++)
                mesh->getBuffer(buf_i)->uploadData();
        }
    }

    std::shared_lock drawlist_lock(drawlist_mutex);

    for (auto &l : layers) {
        l.first.setupRenderState(client);

        for (auto &mesh_l : l.second) {
            auto mesh = meshes[mesh_l.second];

            if (!mesh)
                continue;
            auto &lp = mesh_l.first;

            if (!lp.buf_ref->getVAO())
                continue;

            matrix4 t;
            v3f meshPos = mesh->getAbsoluteMeshPos();
            t.setTranslation(meshPos - intToFloat(cameraOffset, BS));
            t.setRotationDegrees(mesh->getRotation());
            rnd->setTransformMatrix(TMatrix::World, t);

            rnd->draw(lp.buf_ref, render::PT_TRIANGLES, lp.offset, lp.count);
        }
    }

    if (draw_control.show_wireframe)
        ctxt->setPolygonMode(render::CM_FRONT_AND_BACK, render::PM_FILL);
}

/*void DistanceSortedDrawList::renderShadows(TileLayer &override_layer)
{
    auto rndsys = client->getRenderSystem();
    auto rnd = rndsys->getRenderer();
    auto ctxt = rnd->getContext();

    v3s16 cameraOffset = camera->getOffset();

    MutexAutoLock list_lock(shadow_meshes_mutex);
    for (auto &m : shadow_meshes) {
        matrix4 t;
        v3f center = m->getBoundingSphereCenter();
        t.setTranslation(center - intToFloat(cameraOffset, BS));
        rnd->setTransformMatrix(TMatrix::World, t);

        for (auto &l : m->getAllLayers()) {
            override_layer.material_type = l.first.material_type;
            override_layer.setupRenderState(client);

            if (translucent_foliage && l.first.material_type & TILE_MATERIAL_WAVING_LEAVES)
                ctxt->setCullMode(render::CM_FRONT);

            rnd->draw(l.second.buf_ref, render::PT_TRIANGLES, l.second.offset, l.second.count);
        }
    }
}*/

void *DrawListUpdateThread::run()
{
    BEGIN_DEBUG_EXCEPTION_HANDLER

    while (!stopRequested()) {
        //drawlist->resortShadowList();
        drawlist->updateList();
        sleep_ms(50);
    }

    END_DEBUG_EXCEPTION_HANDLER

    return nullptr;
}

bool DistanceSortedDrawList::isMeshOccluded(LayeredMesh *mesh, u16 mesh_size)
{
    return false;
}
