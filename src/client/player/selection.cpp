#include "selection.h"
#include "client/render/renderer.h"
#include "client/render/rendersystem.h"
#include "client/media/resource.h"
#include "settings.h"
#include "client/mesh/meshoperations.h"
#include "util/numeric.h"
#include "client/render/batcher3d.h"
#include "client/core/client.h"
#include "client/player/localplayer.h"
#include "client/player/playercamera.h"
#include "client/mesh/layeredmesh.h"
#include "client/render/tilelayer.h"
#include "client/render/drawlist.h"
#include "client/render/atlas.h"

SelectionMesh::SelectionMesh(RenderSystem *_rndsys, ResourceCache *_cache)
    : rndsys(_rndsys), cache(_cache)
{
    v3f selectionbox_color = g_settings->getV3F("selectionbox_color").value_or(v3f());
    base_color.R(std::round(selectionbox_color.X));
    base_color.G(std::round(selectionbox_color.Y));
    base_color.B(std::round(selectionbox_color.Z));
    base_color.A(255);

    std::string mode_setting = g_settings->get("node_highlighting");

    if (mode_setting == "halo") {
        mode = HIGHLIGHT_HALO;
    } else if (mode_setting == "none") {
        mode = HIGHLIGHT_NONE;
    } else {
        mode = HIGHLIGHT_BOX;
    }

    if (mode == HIGHLIGHT_HALO)
        halo_img = cache->get<img::Image>(ResourceType::IMAGE, "halo.png");
    else if (mode == HIGHLIGHT_BOX)
        thickness = std::clamp<f32>((f32)g_settings->getS16("selectionbox_width"), 1.0f, 5.0f);
}

void SelectionMesh::updateMesh(const v3f &new_pos, const v3s16 &camera_offset,
    const std::vector<aabbf> &new_boxes, DistanceSortedDrawList *drawlist)
{
    if (new_pos == pos || mode == HIGHLIGHT_NONE)
        return;

    drawlist->removeLayeredMesh(mesh);
    mesh = nullptr;

    if (new_boxes.empty())
        return;

    boxes = new_boxes;
    pos = new_pos;
    pos_with_offset = pos - intToFloat(camera_offset, BS);

    aabbf max_box;

    for (auto &box : boxes)
        max_box.addInternalBox(box);

    mesh = new LayeredMesh(v3f(max_box.getRadius()), pos_with_offset);
    mesh->getRotation() = rotation;

    TileLayer layer;
    layer.thing = RenderThing::BOX;
    layer.alpha_discard = 1;
    layer.material_flags = MATERIAL_FLAG_TRANSPARENT;
    layer.use_default_shader = true;

    // Use single halo box instead of multiple overlapping boxes.
    // Temporary solution - problem can be solved with multiple
    // rendering targets, or some method to remove inner surfaces.
    // Thats because of halo transparency.

    MeshBuffer *buf;
    if (mode == HIGHLIGHT_HALO) {
        aabbf halo_box = default_halo_box;

        for (const auto &sbox : boxes)
            halo_box.addInternalBox(sbox);

        layer.tile_ref = halo_img;
        buf = MeshOperations::convertNodeboxesToMesh({halo_box}, nullptr, 0.5f);

        rndsys->getPool(true)->updateAllMeshUVs(buf, halo_img);

        MeshOperations::colorizeMesh(buf, light_color);
        img::color8 face_color(img::PF_RGBA8,
            light_color.R() * 1.5f,
            light_color.G() * 1.5f,
            light_color.B() * 1.5f,
            0);
        MeshOperations::setMeshColorByNormal(buf, face_normal, face_color);
    }
    else {
        img::color8 res_color = base_color * light_color;
        res_color.A(255);

        buf = new MeshBuffer(8 * boxes.size(), 36 * boxes.size());
        for (auto &box : boxes)
            Batcher3D::lineBox(buf, box, res_color);

        layer.line_thickness = thickness;
    }

    buf->uploadData();
    mesh->addNewBuffer(buf);

    LayeredMeshPart mesh_p;
    mesh_p.count = buf->getIndexCount();
    mesh->addNewLayer(buf, layer, mesh_p);

    mesh->splitTransparentLayers();

    drawlist->addLayeredMesh(mesh);
}


BlockBounds::BlockBounds(RenderSystem *_rndsys)
    : rndsys(_rndsys)//, mesh(std::make_unique<MeshBuffer>(false, render::DefaultVType, render::MeshUsage::DYNAMIC))
{
    thickness = std::clamp<f32>((f32)g_settings->getS16("selectionbox_width"), 1.0f, 5.0f);
}

BlockBounds::Mode BlockBounds::toggle(Client *client, DistanceSortedDrawList *drawlist)
{
    mode = static_cast<Mode>(mode + 1);

    if (mode > BLOCK_BOUNDS_NEAR) {
       mode = BLOCK_BOUNDS_OFF;
    }

    updateMesh(client, drawlist);

    return mode;
}

void BlockBounds::updateMesh(Client *client, DistanceSortedDrawList *drawlist)
{
    drawlist->removeLayeredMesh(mesh);
    mesh = nullptr;

    if (mode == BLOCK_BOUNDS_OFF)
        return;

    u16 mesh_chunk_size = std::max<u16>(1, g_settings->getU16("client_mesh_chunk"));

    auto player = client->getEnv().getLocalPlayer();
    v3s16 block_pos = getContainerPos(player->getStandingNodePos(), MAP_BLOCKSIZE);

    v3f cam_offset = intToFloat(player->getCamera()->getOffset(), BS);

    v3f half_node = v3f(BS, BS, BS) / 2.0f;
    v3f base_corner = intToFloat(block_pos * MAP_BLOCKSIZE, BS) - cam_offset - half_node;

    s16 radius = mode == BLOCK_BOUNDS_NEAR ?
        rangelim(g_settings->getU16("show_block_bounds_radius_near"), 0, 1000) : 0;

    mesh = new LayeredMesh(v3f(radius * MAP_BLOCKSIZE * BS),
        intToFloat(block_pos * MAP_BLOCKSIZE, BS) - cam_offset);
    MeshBuffer *buf = new MeshBuffer(4 * radius * radius * 3, 0, false);

    for (s16 x = -radius; x <= radius + 1; x++)
        for (s16 y = -radius; y <= radius + 1; y++) {
            // Red for mesh chunk edges, yellow for other block edges.
            auto choose_color = [&](s16 x_base, s16 y_base) {
                // See also MeshGrid::isMeshPos().
                // If the block is mesh pos, it means it's at the (-,-,-) corner of
                // the mesh. And we're drawing a (-,-) edge of this block. Hence,
                // it is an edge of the mesh grid.
                return (x + x_base) % mesh_chunk_size == 0
                               && (y + y_base) % mesh_chunk_size == 0 ?
                           img::color8(img::PF_RGBA8, 255, 0, 0, 255) :
                           img::color8(img::PF_RGBA8, 255, 255, 0, 255);
            };

            v3f pmin = v3f(x, y,    -radius) * MAP_BLOCKSIZE * BS;
            v3f pmax = v3f(x, y, 1 + radius) * MAP_BLOCKSIZE * BS;

            Batcher3D::line(buf,
                base_corner + v3f(pmin.X, pmin.Y, pmin.Z),
                base_corner + v3f(pmax.X, pmax.Y, pmax.Z),
                choose_color(block_pos.X, block_pos.Y)
                );
            Batcher3D::line(buf,
                base_corner + v3f(pmin.X, pmin.Z, pmin.Y),
                base_corner + v3f(pmax.X, pmax.Z, pmax.Y),
                choose_color(block_pos.X, block_pos.Z)
                );
            Batcher3D::line(buf,
                base_corner + v3f(pmin.Z, pmin.X, pmin.Y),
                base_corner + v3f(pmax.Z, pmax.X, pmax.Y),
                choose_color(block_pos.Y, block_pos.Z)
                );
        }

    TileLayer layer;
    layer.thing = RenderThing::BOX;
    layer.alpha_discard = 1;
    layer.material_flags = MATERIAL_FLAG_TRANSPARENT;
    layer.use_default_shader = true;
    layer.line_thickness = thickness;

    buf->uploadData();
    mesh->addNewBuffer(buf);

    LayeredMeshPart mesh_p;
    mesh_p.count = buf->getIndexCount();
    mesh->addNewLayer(buf, layer, mesh_p);

    mesh->splitTransparentLayers();

    drawlist->addLayeredMesh(mesh);
}
