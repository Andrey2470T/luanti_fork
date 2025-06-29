#include "selection.h"
#include "client/render/renderer.h"
#include "client/media/resource.h"
#include "settings.h"
#include "client/render//meshoperations.h"

SelectionMesh::SelectionMesh(Renderer *_rnd, ResourceCache *_cache)
    : rnd(_rnd), cache(_cache), mesh(std::make_unique<MeshBuffer>(true))
{
    v3f selectionbox_color = g_settings->getV3F("selectionbox_color").value_or(v3f());
    color.R(std::round(selectionbox_color.X));
    color.G(std::round(selectionbox_color.Y));
    color.B(std::round(selectionbox_color.Z));
    color.A(255);

    std::string mode_setting = g_settings->get("node_highlighting");

    if (mode_setting == "halo") {
        mode = HIGHLIGHT_HALO;
    } else if (mode_setting == "none") {
        mode = HIGHLIGHT_NONE;
    } else {
        mode = HIGHLIGHT_BOX;
    }

    if (mode == HIGHLIGHT_HALO)
        halo_tex = cache->getOrLoad<render::Texture2D>(ResourceType::TEXTURE, "halo.png")->data.get();
    else if (mode == HIGHLIGHT_BOX)
        thickness = std::clamp<f32>((f32)g_settings->getS16("selectionbox_width"), 1.0f, 5.0f);
}

void SelectionMesh::updateSelectionMesh(const v3s16 &offset)
{
    camera_offset = offset;
    if (mode != HIGHLIGHT_HALO)
        return;



    if (boxes.empty()) {
        // No pointed object
        return;
    }

    // New pointed object, create new mesh.

    // Texture UV coordinates for selection boxes
    static f32 texture_uv[24] = {
        0,0,1,1,
        0,0,1,1,
        0,0,1,1,
        0,0,1,1,
        0,0,1,1,
        0,0,1,1
    };

    // Use single halo box instead of multiple overlapping boxes.
    // Temporary solution - problem can be solved with multiple
    // rendering targets, or some method to remove inner surfaces.
    // Thats because of halo transparency.

    aabb3f halo_box(100.0, 100.0, 100.0, -100.0, -100.0, -100.0);
    m_halo_boxes.clear();

    for (const auto &selection_box : m_selection_boxes) {
        halo_box.addInternalBox(selection_box);
    }

    m_halo_boxes.push_back(halo_box);
    m_selection_mesh = convertNodeboxesToMesh(
        m_halo_boxes, texture_uv, 0.5);
}
