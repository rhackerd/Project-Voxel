#include "log/log.hpp"
#include "render/graphics.hpp"
#include "render/obj.hpp"
#include <SDL3/SDL_gpu.h>
#include <glm/gtc/type_ptr.hpp>
#include "stb/stb_image.h"

namespace N::Graphics {

    bool Object::LoadPBRTextures(cgltf_data* data) {
        LOG_INFO("  - Loading PBR textures");
        size_t meshIdx = 0;

        auto loadTex = [&](Texture& tex, cgltf_image* image) {
            int w, h, ch;
            stbi_uc* pixels = nullptr;

            if (image->buffer_view) {
                void* data = (char*)image->buffer_view->buffer->data + image->buffer_view->offset;
                pixels = stbi_load_from_memory((stbi_uc*)data, (int)image->buffer_view->size, &w, &h, &ch, 4);
            } else if (image->uri) {
                pixels = stbi_load(image->uri, &w, &h, &ch, 4);
            }

            if (!pixels) return;

            tex.Init(m_device, w, h);
            tex.Upload(m_device, pixels);
            stbi_image_free(pixels);
        };

        for (size_t mi = 0; mi < data->meshes_count; mi++) {

            cgltf_mesh& cgmesh = data->meshes[mi];

            for (size_t pi = 0; pi < cgmesh.primitives_count; pi++) {
                cgltf_primitive& prim = cgmesh.primitives[pi];
                cgltf_material* mat = prim.material;

                if (!mat || meshIdx >= m_meshes.size()) {meshIdx++; continue;}

                Mesh& mesh = m_meshes[meshIdx];

                if (mat->pbr_metallic_roughness.base_color_texture.texture)
                    loadTex(mesh.textures.albedo, mat->pbr_metallic_roughness.base_color_texture.texture->image);

                if (mat->normal_texture.texture)
                    loadTex(mesh.textures.normal, mat->normal_texture.texture->image);

                if (mat->pbr_metallic_roughness.metallic_roughness_texture.texture)
                    loadTex(mesh.textures.metalRough, mat->pbr_metallic_roughness.metallic_roughness_texture.texture->image);

                if (mat->emissive_texture.texture)
                    loadTex(mesh.textures.emissive, mat->emissive_texture.texture->image);
                
                meshIdx++;
            }

        }

        LOG_INFO("  - PBR textures loaded");

        return true;
    }
    
    void Object::GatherGeometry(cgltf_data* data, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {
        LOG_INFO("  - Gathering geometry");
        for (size_t n = 0; n < data->nodes_count; n++) {
            cgltf_node* node = &data->nodes[n];
            if (!node->mesh) continue;
            LoadMesh(node, vertices, indices);
        }
    }

    void Object::UploadToGPU(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {
        LOG_INFO("  - Uploading to GPU");
        CreateModelBuffers(vertices.size(), indices.size());

        Uint32 vbuf_size = sizeof(Vertex)   * vertices.size();
        Uint32 ibuf_size = sizeof(uint32_t) * indices.size();

        SDL_GPUTransferBufferCreateInfo tbuf_info{};
        tbuf_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        tbuf_info.size  = vbuf_size + ibuf_size;
        SDL_GPUTransferBuffer* tbuf = SDL_CreateGPUTransferBuffer(m_device, &tbuf_info);

        void* map = SDL_MapGPUTransferBuffer(m_device, tbuf, false);
        memcpy(map, vertices.data(), vbuf_size);
        memcpy((Uint8*)map + vbuf_size, indices.data(), ibuf_size);
        SDL_UnmapGPUTransferBuffer(m_device, tbuf);

        SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(m_device);
        SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd);

        SDL_GPUTransferBufferLocation src{};
        SDL_GPUBufferRegion dst{};

        src.transfer_buffer = tbuf; src.offset = 0;
        dst.buffer = model.vbuf; dst.offset = 0; dst.size = vbuf_size;
        SDL_UploadToGPUBuffer(copy, &src, &dst, false);

        src.offset = vbuf_size;
        dst.buffer = model.ibuf; dst.offset = 0; dst.size = ibuf_size;
        SDL_UploadToGPUBuffer(copy, &src, &dst, false);

        SDL_EndGPUCopyPass(copy);
        SDL_SubmitGPUCommandBuffer(cmd);
        SDL_ReleaseGPUTransferBuffer(m_device, tbuf);

        model.indexCount = (uint32_t)indices.size();
    }

    void Object::LoadMesh(cgltf_node* node, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {
        cgltf_float local[16];
        cgltf_node_transform_local(node, local);

        glm::mat4 transform = glm::make_mat4(local);
        glm::mat3 normalMat = glm::transpose(glm::inverse(glm::mat3(transform)));

        for (size_t p = 0; p < node->mesh->primitives_count; p++) {
            auto& prim = node->mesh->primitives[p];
            if (prim.type != cgltf_primitive_type_triangles) continue;

            cgltf_accessor* idx_acc  = prim.indices;
            cgltf_accessor* pos_acc  = nullptr;
            cgltf_accessor* norm_acc = nullptr;
            cgltf_accessor* uv_acc   = nullptr;
            cgltf_accessor* tan_acc  = nullptr;

            for (size_t a = 0; a < prim.attributes_count; a++) {
                auto& attr = prim.attributes[a];
                if (attr.type == cgltf_attribute_type_position) pos_acc  = attr.data;
                if (attr.type == cgltf_attribute_type_normal)   norm_acc = attr.data;
                if (attr.type == cgltf_attribute_type_texcoord) uv_acc   = attr.data;
                if (attr.type == cgltf_attribute_type_tangent)  tan_acc  = attr.data;
            }

            

            if (!pos_acc || !idx_acc) continue;

            // track offsets for this mesh slice
            uint32_t index_offset  = (uint32_t)indices.size();
            uint32_t vertex_offset = (uint32_t)vertices.size();

            uint32_t base_vertex = vertex_offset;
            for (size_t i = 0; i < idx_acc->count; i++)
                indices.push_back(base_vertex + (uint32_t)cgltf_accessor_read_index(idx_acc, i));

            vertices.resize(vertex_offset + pos_acc->count);

            for (size_t i = 0; i < pos_acc->count; i++) {
                Vertex& v = vertices[vertex_offset + i];

                

                float p[3] = {};
                cgltf_accessor_read_float(pos_acc, i, p, 3);

                v.position = glm::vec3(p[0], p[1], p[2]);

                if (norm_acc) {
                    float nm[3] = {};
                    cgltf_accessor_read_float(norm_acc, i, nm, 3);
                    v.normal = glm::normalize(normalMat * glm::vec3(nm[0], nm[1], nm[2]));
                }

                if (tan_acc) {
                    float t[4] = {};
                    cgltf_accessor_read_float(tan_acc, i, t, 4);
                    v.tangent = glm::vec4(t[0], t[1], t[2], t[3]);
                }

                

                if (uv_acc)
                    cgltf_accessor_read_float(uv_acc, i, &v.uv.x, 2);
            }


            // register mesh slice
            Mesh mesh{};
            mesh.indexOffset  = index_offset;
            mesh.indexCount   = (uint32_t)idx_acc->count;
            mesh.vertexOffset = vertex_offset;
            mesh.nodeTransform = glm::make_mat4(local);
            m_meshes.push_back(mesh);
            
        }
    }
};