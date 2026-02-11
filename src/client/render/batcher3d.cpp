#include "batcher3d.h"
#include "client/mesh/defaultVertexTypes.h"
#include "Utils/Plane3D.h"
#include "client/mesh/lighting.h"
#include "client/mesh/meshoperations.h"

/*bool Batcher3D::applyFaceShading = false;
bool Batcher3D::smoothLighting = false;
img::color8 Batcher3D::curLightColor = img::white;
LightFrame Batcher3D::curLightFrame;
u8 Batcher3D::curLightSource;*/

void Batcher3D::vertex(MeshBuffer *buf, v3f pos,
    const img::color8 &color, const v3f &normal, v2f uv)
{

    /*img::color8 newColor = color;
    if (smoothLighting)
        newColor = blendLightColor(pos, normal, curLightFrame, curLightSource);
    else
        newColor *= curLightColor;

    if (applyFaceShading)
        MeshOperations::applyFacesShading(newColor, normal);*/

    auto vType = buf->getVertexType();
    if (vType.Name == "Node3D")
        NVT(buf, pos, color, normal, uv);
    else if (vType.Name == "TwoColorNode3D")
        TCNVT(buf, pos, color, normal, uv);
    else if (vType.Name == "AnimatedObject3D")
        AOVT(buf, pos, color, normal, uv);
    else if (vType.Name == "Skybox3D")
        SBVT(buf, pos, color, normal, uv);
    else
        SVT(buf, pos, color, normal, uv);
}

void Batcher3D::index(MeshBuffer *buf, u32 index)
{
    buf->setIndexAt(index);
}

void Batcher3D::line(MeshBuffer *buf, const v3f &startPos, const v3f &endPos,
    const img::color8 &color)
{
    if (buf->hasIBO()) {
        u32 curVIndex = buf->getVertexOffset();
        index(buf, curVIndex);
        index(buf, curVIndex+1);
    }

    vertex(buf, startPos, color);
    vertex(buf, endPos, color);
}

void Batcher3D::triangle(MeshBuffer *buf, const std::array<v3f, 3> &positions,
    const img::color8 &color, const std::array<v2f, 3> &uvs, const std::array<v3f, 4> &normals)
{
    if (buf->hasIBO()) {
        u32 curVIndex = buf->getVertexOffset();
        index(buf, curVIndex);
        index(buf, curVIndex+1);
        index(buf, curVIndex+2);
    }
    //std::array<v3f, 4> used_normals;

    /*if (normals.has_value())
        used_normals = normals.value();
    else {
        v3f normal = plane3f(positions[0], positions[1], positions[2]).Normal;
        used_normals = {normal, normal, normal, normal};
    }*/
    vertex(buf, positions[0], color, normals[0], uvs[0]);
    vertex(buf, positions[1], color, normals[1], uvs[1]);
    vertex(buf, positions[2], color, normals[2], uvs[2]);
}

enum class RotationAxis : u8
{
    X,
    Y,
    Z
};

inline void rotateFace(v3f &pos1, v3f &pos2, v3f &pos3, v3f &pos4, RotationAxis axis, f64 degrees)
{
    switch(axis) {
    case RotationAxis::X: {
        pos1.rotateYZBy(degrees);
        pos2.rotateYZBy(degrees);
        pos3.rotateYZBy(degrees);
        pos4.rotateYZBy(degrees);
        break;
    }
    case RotationAxis::Y: {
        pos1.rotateXZBy(degrees);
        pos2.rotateXZBy(degrees);
        pos3.rotateXZBy(degrees);
        pos4.rotateXZBy(degrees);
        break;
    }
    case RotationAxis::Z: {
        pos1.rotateXYBy(degrees);
        pos2.rotateXYBy(degrees);
        pos3.rotateXYBy(degrees);
        pos4.rotateXYBy(degrees);
        break;
    }
    }
}

void Batcher3D::triangle(MeshBuffer *buf, const std::array<v2f, 3> &positions, const v3f &rotation,
    const img::color8 &color, const std::array<v2f, 3> &uvs, const std::array<v3f, 4> &normals)
{
    std::array<v3f, 3> positions3d = {
        v3f(positions[0].X, positions[0].Y, 0.0f),
        v3f(positions[1].X, positions[1].Y, 0.0f),
        v3f(positions[2].X, positions[2].Y, 0.0f)
    };

    v3f fakePos;

    if (rotation.X != 0.0f)
        rotateFace(positions3d[0], positions3d[1], positions3d[2], fakePos, RotationAxis::X, rotation.X);
    if (rotation.Y != 0.0f)
        rotateFace(positions3d[0], positions3d[1], positions3d[2], fakePos, RotationAxis::Y, rotation.Y);
    if (rotation.Z != 0.0f)
        rotateFace(positions3d[0], positions3d[1], positions3d[2], fakePos, RotationAxis::Z, rotation.Z);

    triangle(buf, positions3d, color, uvs, normals);
}

void Batcher3D::face(MeshBuffer *buf, const std::array<v3f, 4> &positions,
    const std::array<img::color8, 4> &colors, const rectf &uvs, const std::array<v3f, 4> &normals)
{
    //std::array<v3f, 4> used_normals;

    /* (normals.has_value())
        used_normals = normals.value();
    else {
        v3f normal = plane3f(positions[0], positions[1], positions[2]).Normal;
        used_normals = {normal, normal, normal, normal};
    }*/

    if (buf->hasIBO()) {
        std::array<u32, 6> indices = {0, 3, 2, 0, 2, 1};

        u32 curVIndex = buf->getVertexOffset();
        for (u32 i = 0; i < 6; i++)
            index(buf, curVIndex+indices[i]);
    }

    vertex(buf, positions[0], colors[0], normals[0], v2f(uvs.ULC.X, uvs.LRC.Y));
    vertex(buf, positions[1], colors[1], normals[1], uvs.LRC);
    vertex(buf, positions[2], colors[2], normals[2], v2f(uvs.LRC.X, uvs.ULC.Y));
    vertex(buf, positions[3], colors[3], normals[3], uvs.ULC);
}

void Batcher3D::face(MeshBuffer *buf, const rectf &positions, const v3f &rotation,
    const std::array<img::color8, 4> &colors, const rectf &uvs, const std::array<v3f, 4> &normals)
{
    std::array<v3f, 4> positions3d = {
        v3f(positions.ULC.X, positions.LRC.Y, 0.0f),
        v3f(positions.LRC.X, positions.LRC.Y, 0.0f),
        v3f(positions.LRC.X, positions.ULC.Y, 0.0f),
        v3f(positions.ULC.X, positions.ULC.Y, 0.0f)
    };

    if (rotation.X != 0.0f)
        rotateFace(positions3d[0], positions3d[1], positions3d[2], positions3d[3], RotationAxis::X, rotation.X);
    if (rotation.Y != 0.0f)
        rotateFace(positions3d[0], positions3d[1], positions3d[2], positions3d[3], RotationAxis::Y, rotation.Y);
    if (rotation.Z != 0.0f)
        rotateFace(positions3d[0], positions3d[1], positions3d[2], positions3d[3], RotationAxis::Z, rotation.Z);

    face(buf, positions3d, colors, uvs, normals);
}

void Batcher3D::box(MeshBuffer *buf, const aabbf &box, const std::array<img::color8, 24> &colors,
    const std::array<rectf, 6> *uvs, u8 mask)
{
    // Up
    boxFace(buf, BoxFaces::TOP, box, colors, uvs, mask);
    // Down
    boxFace(buf, BoxFaces::BOTTOM, box, colors, uvs, mask);
    // Right
    boxFace(buf, BoxFaces::RIGHT, box, colors, uvs, mask);
    // Left
    boxFace(buf, BoxFaces::LEFT, box, colors, uvs, mask);
    // Back
    boxFace(buf, BoxFaces::BACK, box, colors, uvs, mask);
    // Front
    boxFace(buf, BoxFaces::FRONT, box, colors, uvs, mask);
}

void Batcher3D::boxFace(
    MeshBuffer *buf,
    BoxFaces faceNum,
    const aabbf &box,
    const std::array<img::color8, 24> &colors,
    const std::array<rectf, 6> *uvs,
    u8 mask)
{
    // Auto calculation of uvs
    auto calc_uv = [] (u32 face_w, u32 face_h)
    {
        v2f uv_size{1.0f, 1.0f};
        if (face_w > face_h) {
            uv_size.X = 1.0f;
            uv_size.Y = (f32)face_h / face_w;
        }
        else if (face_w < face_h) {
            uv_size.Y = 1.0f;
            uv_size.X = (f32)face_w / face_h;
        }

        rectf uv(uv_size);

        v2f uv_center = uv.getCenter();
        v2f uv_shift = v2f(0.5f) - uv_center;

        uv += uv_shift;

        return uv;
    };

    v3f box_size = box.getExtent();

    // Up
    if (faceNum == BoxFaces::TOP && (mask & (1 << 0))) {
        rectf up_uv = uvs ? (*uvs)[0] : calc_uv(box_size.X, box_size.Z);
        face(
            buf,
            {
                v3f(box.MinEdge.X, box.MaxEdge.Y, box.MaxEdge.Z), box.MaxEdge,
                v3f(box.MaxEdge.X, box.MaxEdge.Y, box.MinEdge.Z), v3f(box.MinEdge.X, box.MaxEdge.Y, box.MinEdge.Z)
            },
            {colors[0], colors[1], colors[2], colors[3]}, up_uv
        );
    }
    // Down
    else if (faceNum == BoxFaces::BOTTOM && (mask & (1 << 1))) {
        rectf down_uv = uvs ? (*uvs)[1] : calc_uv(box_size.X, box_size.Z);
        face(
            buf,
            {
                box.MinEdge, v3f(box.MaxEdge.X, box.MinEdge.Y, box.MinEdge.Z),
                v3f(box.MaxEdge.X, box.MinEdge.Y, box.MaxEdge.Z), v3f(box.MinEdge.X, box.MinEdge.Y, box.MaxEdge.Z)
            },
            {colors[4], colors[5], colors[6], colors[7]}, down_uv
        );
    }
    // Right
    else if (faceNum == BoxFaces::RIGHT && (mask & (1 << 2))) {
        rectf right_uv = uvs ? (*uvs)[2] : calc_uv(box_size.Z, box_size.Y);
        face(
            buf,
            {
                v3f(box.MaxEdge.X, box.MaxEdge.Y, box.MinEdge.Z), box.MaxEdge,
                v3f(box.MaxEdge.X, box.MinEdge.Y, box.MaxEdge.Z), v3f(box.MaxEdge.X, box.MinEdge.Y, box.MinEdge.Z)
            },
            {colors[8], colors[9], colors[10], colors[11]}, right_uv
        );
    }
    // Left
    else if (faceNum == BoxFaces::LEFT && (mask & (1 << 3))) {
        rectf left_uv = uvs ? (*uvs)[3] : calc_uv(box_size.Z, box_size.Y);
        face(
            buf,
            {
                v3f(box.MinEdge.X, box.MaxEdge.Y, box.MaxEdge.Z), v3f(box.MinEdge.X, box.MaxEdge.Y, box.MinEdge.Z),
                box.MinEdge, v3f(box.MinEdge.X, box.MinEdge.Y, box.MaxEdge.Z)
            },
            {colors[12], colors[13], colors[14], colors[15]}, left_uv
        );
    }
    // Back
    else if (faceNum == BoxFaces::BACK && (mask & (1 << 4))) {
        rectf back_uv = uvs ? (*uvs)[4] : calc_uv(box_size.X, box_size.Y);
        face(
            buf,
            {
                box.MaxEdge, v3f(box.MinEdge.X, box.MaxEdge.Y, box.MaxEdge.Z),
                v3f(box.MinEdge.X, box.MinEdge.Y, box.MaxEdge.Z), v3f(box.MaxEdge.X, box.MinEdge.Y, box.MaxEdge.Z)
            },
            {colors[16], colors[17], colors[18], colors[19]}, back_uv
        );
    }
    // Front
    else if (faceNum == BoxFaces::FRONT && (mask & (1 << 5))) {
        rectf front_uv = uvs ? (*uvs)[5] : calc_uv(box_size.X, box_size.Y);
        face(
            buf,
            {
                v3f(box.MinEdge.X, box.MaxEdge.Y, box.MinEdge.Z), v3f(box.MaxEdge.X, box.MaxEdge.Y, box.MinEdge.Z),
                v3f(box.MaxEdge.X, box.MinEdge.Y, box.MinEdge.Z), box.MinEdge
            },
            {colors[20], colors[21], colors[22], colors[23]}, front_uv
        );
    }
}

void Batcher3D::lineBox(MeshBuffer *buf, const aabbf &box, const img::color8 &color)
{
    assert(buf->hasIBO());

    // Draws the lines as flattened triangles to be transparenct-sorted in the drawlist
    std::array<u32, 24> indices = {
        0, 1,  1, 5,  5, 4,  4, 0, // front
        2, 3,  3, 7,  7, 6,  6, 2, // back
        1, 3,  0, 2, // left side
        5, 7,  4, 6  // right side
    };

    u32 curVIndex = buf->getVertexOffset();
    for (u32 i : indices)
        index(buf, curVIndex+i);

    std::array<v3f, 8> edges;
    box.getEdges(edges.data());

    for (auto edge : edges)
        vertex(buf, edge, color);
}
