#include "selection.h"
#include "client/render/renderer.h"
#include "client/media/resource.h"
#include "settings.h"
#include "client/mesh/meshoperations.h"
#include "util/numeric.h"
#include "client/render/batcher3d.h"
#include "client/client.h"
#include "client/player/localplayer.h"
#include "client/player/playercamera.h"

SelectionMesh::SelectionMesh(Renderer *_rnd, ResourceCache *_cache)
    : rnd(_rnd), cache(_cache), mesh(std::make_unique<MeshBuffer>(true, render::DefaultVType, render::MeshUsage::DYNAMIC))
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
        halo_tex = cache->get<render::Texture2D>(ResourceType::TEXTURE, "halo.png");
    else if (mode == HIGHLIGHT_BOX)
        thickness = std::clamp<f32>((f32)g_settings->getS16("selectionbox_width"), 1.0f, 5.0f);
}

void SelectionMesh::draw() const
{
    if (mode == HIGHLIGHT_NONE)
        return;

    render::PrimitiveType pType = mode == HIGHLIGHT_HALO ? render::PT_TRIANGLES : render::PT_LINES;
    rnd->draw(mesh.get(), pType);
}

void SelectionMesh::updateMesh()
{
    if (boxes.empty() || mode == HIGHLIGHT_NONE) {
        // No pointed object
        return;
    }

    Batcher3D::vType = B3DVT_SVT;
    // Use single halo box instead of multiple overlapping boxes.
    // Temporary solution - problem can be solved with multiple
    // rendering targets, or some method to remove inner surfaces.
    // Thats because of halo transparency.

    mesh->clear();

    if (mode == HIGHLIGHT_HALO) {
        aabbf halo_box = default_halo_box;

        for (const auto &sbox : boxes)
            halo_box.addInternalBox(sbox);

        mesh.reset(MeshOperations::convertNodeboxesToMesh({halo_box}, nullptr, 0.5f));

        MeshOperations::colorizeMesh(mesh.get(), light_color);
        img::color8 face_color(img::PF_RGBA8,
            light_color.R() * 1.5f,
            light_color.G() * 1.5f,
            light_color.B() * 1.5f,
            0);
        MeshOperations::setMeshColorByNormal(mesh.get(), face_normal, face_color);

        rnd->setDefaultShader(true);
        rnd->setTexture(halo_tex);

    }
    else {
        img::color8 res_color = base_color * light_color;
        res_color.A(255);

        mesh->reallocateData(8 * boxes.size(), 24 * boxes.size());
        for (auto &box : boxes)
            Batcher3D::appendLineBox(mesh.get(), box, res_color);

        rnd->setDefaultShader();
    }

    mesh->uploadData();

    rnd->setRenderState(true);
    rnd->setDefaultUniforms(thickness, 0, 0.5f, img::BM_COUNT);
    rnd->enableFog(false);

    matrix4 t;
    t.setTranslation(pos_with_offset);
    matrix4 r;
    t.setRotationDegrees(rotation);

    rnd->setTransformMatrix(TMatrix::World, t * r);
}

void SelectionMesh::setPos(const v3f &position, const v3s16 &camera_offset)
{
    pos = position;
    pos_with_offset = position - intToFloat(camera_offset, BS);
}


BlockBounds::BlockBounds(Renderer *_rnd)
    : rnd(_rnd), mesh(std::make_unique<MeshBuffer>(false, render::DefaultVType, render::MeshUsage::DYNAMIC))
{
    thickness = std::clamp<f32>((f32)g_settings->getS16("selectionbox_width"), 1.0f, 5.0f);
}

BlockBounds::Mode BlockBounds::toggle(Client *client)
{
    mode = static_cast<Mode>(mode + 1);

    if (mode > BLOCK_BOUNDS_NEAR) {
       mode = BLOCK_BOUNDS_OFF;
    }

    updateMesh(client);

    return mode;
}

void BlockBounds::draw() const
{
    if (mode == BLOCK_BOUNDS_OFF)
        return;

    rnd->draw(mesh.get(), render::PT_LINES);
}

void BlockBounds::updateMesh(Client *client)
{
    if (mode == BLOCK_BOUNDS_OFF)
        return;

    Batcher3D::vType = B3DVT_SVT;
    u16 mesh_chunk_size = std::max<u16>(1, g_settings->getU16("client_mesh_chunk"));

    v3s16 block_pos = getContainerPos(client->getEnv().getLocalPlayer()->getStandingNodePos(), MAP_BLOCKSIZE);

    v3f cam_offset = intToFloat(client->getCamera()->getOffset(), BS);

    v3f half_node = v3f(BS, BS, BS) / 2.0f;
    v3f base_corner = intToFloat(block_pos * MAP_BLOCKSIZE, BS) - cam_offset - half_node;

    s16 radius = mode == BLOCK_BOUNDS_NEAR ?
        rangelim(g_settings->getU16("show_block_bounds_radius_near"), 0, 1000) : 0;

    mesh->clear();
    mesh->reallocateData(4 * radius * radius * 3);
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

            Batcher3D::appendLine(mesh.get(),
                base_corner + v3f(pmin.X, pmin.Y, pmin.Z),
                base_corner + v3f(pmax.X, pmax.Y, pmax.Z),
                choose_color(block_pos.X, block_pos.Y)
                );
            Batcher3D::appendLine(mesh.get(),
                base_corner + v3f(pmin.X, pmin.Z, pmin.Y),
                base_corner + v3f(pmax.X, pmax.Z, pmax.Y),
                choose_color(block_pos.X, block_pos.Z)
                );
            Batcher3D::appendLine(mesh.get(),
                base_corner + v3f(pmin.Z, pmin.X, pmin.Y),
                base_corner + v3f(pmax.Z, pmax.X, pmax.Y),
                choose_color(block_pos.Y, block_pos.Z)
                );
        }

    mesh->uploadData();

    rnd->setRenderState(true);
    rnd->setDefaultShader();
    rnd->setDefaultUniforms(thickness, 0, 0.5f, img::BM_COUNT);
    rnd->enableFog(false);
}
