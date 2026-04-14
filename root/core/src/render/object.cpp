#include "render/core.hpp"
#include "render/graphics.hpp"
#include "render/obj.hpp"
#include <SDL3/SDL_gpu.h>
#include <cstring>
#include <vector>
#include <glm/gtc/type_ptr.hpp>


namespace N::Graphics {
    void Object::Init() {
        // Just load a box
    }

    void Object::Shutdown() {
        // Textures
        for (auto mesh : m_meshes) {
            mesh.textures.albedo.Destroy(m_device);
            mesh.textures.normal.Destroy(m_device);
            mesh.textures.metalRough.Destroy(m_device);
            mesh.textures.emissive.Destroy(m_device);  
        }

        DestroyModelBuffers();
    };

    void Object::CreateModelBuffers(size_t vert_num, size_t ind_num) {
        SDL_GPUBufferCreateInfo vbuf_info{};
        vbuf_info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
        vbuf_info.size  = sizeof(Vertex) * vert_num;
        model.vbuf = SDL_CreateGPUBuffer(m_device, &vbuf_info);

        SDL_GPUBufferCreateInfo ibuf_info{};
        ibuf_info.usage = SDL_GPU_BUFFERUSAGE_INDEX;
        ibuf_info.size  = sizeof(uint32_t) * ind_num;
        model.ibuf = SDL_CreateGPUBuffer(m_device, &ibuf_info);
    }

    void Object::DestroyModelBuffers() {
        if (model.vbuf) SDL_ReleaseGPUBuffer(m_device, model.vbuf);
        if (model.ibuf) SDL_ReleaseGPUBuffer(m_device, model.ibuf);
        model.vbuf = nullptr;
        model.ibuf = nullptr;
    };


    void Object::LoadModel(const char* path) {
        std::string full_path = MODELS_PATH + path;
        DestroyModelBuffers();
        LOG_INFO("Loading model: {}", full_path);

        cgltf_options options{};
        cgltf_data* data = nullptr;

        if (cgltf_parse_file(&options, full_path.c_str(), &data) != cgltf_result_success) return;
        if (cgltf_load_buffers(&options, data, full_path.c_str()) != cgltf_result_success) {
            cgltf_free(data);
            return;
        }

        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        GatherGeometry(data, vertices, indices);
        LoadPBRTextures(data);
        cgltf_free(data);

        if (vertices.empty() || indices.empty()) return;

        bool hasTangents = false;
        for (auto& v : vertices) {
            if (glm::length(glm::vec3(v.tangent)) > 0.0f) {
                hasTangents = true;
                break;
            }
        }

        if (!hasTangents) {
            LOG_INFO("  - Generating tangents (MikkTSpace)");
            GenerateTangents(vertices, indices);
        }

        UploadToGPU(vertices, indices);
        LOG_INFO("  - Done");
    }

    void Object::InitFallback() {
        uint32_t white = 0xFFFFFFFF;
        SDL_GPUTextureCreateInfo ti{};
        ti.type = SDL_GPU_TEXTURETYPE_2D;
        ti.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        ti.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        ti.width = 1; ti.height = 1;
        ti.layer_count_or_depth = 1; ti.num_levels = 1;
        m_fallbackTex = SDL_CreateGPUTexture(m_device, &ti);

        SDL_GPUTransferBufferCreateInfo tbi{};
        tbi.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        tbi.size = 4;
        auto* tb = SDL_CreateGPUTransferBuffer(m_device, &tbi);
        void* map = SDL_MapGPUTransferBuffer(m_device, tb, false);
        memcpy(map, &white, 4);
        SDL_UnmapGPUTransferBuffer(m_device, tb);

        auto* cmd = SDL_AcquireGPUCommandBuffer(m_device);
        auto* cp = SDL_BeginGPUCopyPass(cmd);
        SDL_GPUTextureTransferInfo src{}; src.transfer_buffer = tb;
        SDL_GPUTextureRegion dst{}; dst.texture = m_fallbackTex; dst.w=1; dst.h=1; dst.d=1;
        SDL_UploadToGPUTexture(cp, &src, &dst, false);
        SDL_EndGPUCopyPass(cp);
        SDL_SubmitGPUCommandBuffer(cmd);
        SDL_ReleaseGPUTransferBuffer(m_device, tb);
    }


    void Object::Draw(SDL_GPURenderPass* pass, SDL_GPUCommandBuffer* cmd) {
        SDL_GPUBufferBinding vbind{.buffer = model.vbuf, .offset = 0};
        SDL_BindGPUVertexBuffers(pass, 0, &vbind, 1);

        SDL_GPUBufferBinding ibind{.buffer = model.ibuf, .offset = 0};
        SDL_BindGPUIndexBuffer(pass, &ibind, SDL_GPU_INDEXELEMENTSIZE_32BIT);


        for (auto& mesh : m_meshes) {

            ObjPC pc = getMeshPC(mesh);
            SDL_PushGPUVertexUniformData(cmd, 1, &pc, sizeof(ObjPC));

            SDL_GPUTextureSamplerBinding albedoBind = {
                mesh.textures.albedo.IsValid() ? mesh.textures.albedo.Get() : m_fallbackTex,
                m_samplers.linear
            };
            SDL_BindGPUFragmentSamplers(pass, 0, &albedoBind, 1);

            SDL_GPUTextureSamplerBinding normalBind = {
                mesh.textures.normal.IsValid() ? mesh.textures.normal.Get() : m_fallbackTex,
                m_samplers.linear
            };
            SDL_BindGPUFragmentSamplers(pass, 1, &normalBind, 1);
            SDL_DrawGPUIndexedPrimitives(pass, mesh.indexCount, 1, mesh.indexOffset, 0, 0);

            // FIX: Binding 1 does not recieve it's own sampler. Works on my machine though
        }
    }
};