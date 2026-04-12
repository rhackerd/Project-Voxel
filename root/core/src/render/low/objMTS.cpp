#include "mikktspace.h"
#include "render/obj.hpp"

#include <mikktspace.h>

struct MikkTSpaceUserData {
    std::vector<N::Graphics::Vertex>& vertices;
    std::vector<uint32_t>& indices;
};

static int mikkGetNumFaces(const SMikkTSpaceContext* ctx) {
    auto* data = (MikkTSpaceUserData*)ctx->m_pUserData;
    return (int)(data->indices.size() / 3);
}

static int mikkGetNumVerticesOfFace(const SMikkTSpaceContext*, int) {
    return 3;
}

static void mikkGetPosition(const SMikkTSpaceContext* ctx, float out[], int face, int vert) {
    auto* data = (MikkTSpaceUserData*)ctx->m_pUserData;
    auto& v = data->vertices[data->indices[face * 3 + vert]];
    out[0] = v.position.x; out[1] = v.position.y; out[2] = v.position.z;
}

static void mikkGetNormal(const SMikkTSpaceContext* ctx, float out[], int face, int vert) {
    auto* data = (MikkTSpaceUserData*)ctx->m_pUserData;
    auto& v = data->vertices[data->indices[face * 3 + vert]];
    out[0] = v.normal.x; out[1] = v.normal.y; out[2] = v.normal.z;
}

static void mikkGetTexCoord(const SMikkTSpaceContext* ctx, float out[], int face, int vert) {
    auto* data = (MikkTSpaceUserData*)ctx->m_pUserData;
    auto& v = data->vertices[data->indices[face * 3 + vert]];
    out[0] = v.uv.x; out[1] = v.uv.y;
}

static void mikkSetTSpaceBasic(const SMikkTSpaceContext* ctx, const float tan[], float sign, int face, int vert) {
    auto* data = (MikkTSpaceUserData*)ctx->m_pUserData;
    auto& v = data->vertices[data->indices[face * 3 + vert]];
    v.tangent = glm::vec4(tan[0], tan[1], tan[2], sign);
}

void N::Graphics::Object::GenerateTangents(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {
    MikkTSpaceUserData userData{vertices, indices};

    SMikkTSpaceInterface iface{};
    iface.m_getNumFaces = mikkGetNumFaces;
    iface.m_getNumVerticesOfFace = mikkGetNumVerticesOfFace;
    iface.m_getPosition = mikkGetPosition;
    iface.m_getNormal = mikkGetNormal;
    iface.m_getTexCoord = mikkGetTexCoord;
    iface.m_setTSpaceBasic = mikkSetTSpaceBasic;

    SMikkTSpaceContext ctx{};
    ctx.m_pInterface = &iface;
    ctx.m_pUserData = &userData;

    genTangSpaceDefault(&ctx);
}