#include "log/log.hpp"
#include "render/graphics.hpp"
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gpu.h>
#include <fstream>
#include <iterator>
#include <vector>

static std::string ext = "";

namespace N::Graphics {
    SDL_GPUShader* Graphics::LoadShader(const char* file,SDL_GPUShaderStage stage, Uint32 samplers, Uint32 uniform_buffers, Uint32 storage_buffers, Uint32 storage_textures) {
        if (ext == "") {
            // query backend
            auto backend = SDL_GetGPUShaderFormats(m_device);
            if (backend == SDL_GPU_SHADERFORMAT_INVALID) {
                NERROR("Failed to query GPU backend");
                return nullptr;
            }else {
                switch (backend) {
                    case SDL_GPU_SHADERFORMAT_SPIRV:
                        ext = ".spv";
                        break;
                    case SDL_GPU_SHADERFORMAT_DXIL:
                        ext = ".dxil";
                        break;
                    case SDL_GPU_SHADERFORMAT_MSL:
                        ext = ".msl";
                        break;
                    default:
                        NERROR("Unknown GPU backend");
                        return nullptr;
                }
            }
        }


        // Detect Stage from filename
        std::string stage_string = "";
        switch (stage) {
            case SDL_GPU_SHADERSTAGE_VERTEX:
                stage_string = ".vert";
                break;
            case SDL_GPU_SHADERSTAGE_FRAGMENT:
                stage_string = ".frag";
                break;
            default:
                NERROR("Unknown shader stage");
                return nullptr;
        }

        

        NINFO("Loading shader: {}", SHADER_PATH + std::string(file) + stage_string + ext);

        std::ifstream f(SHADER_PATH + std::string(file) + stage_string + ext, std::ios::binary);
        if (!f) {
            NERROR("Failed to open shader file: {}", file);
            return nullptr;
        }

        std::vector<Uint8> code{std::istreambuf_iterator<char>(f), {}};

        SDL_GPUShaderCreateInfo info{};
        info.code = code.data();
        info.code_size = code.size();
        info.entrypoint = "main";
        info.format = SDL_GPU_SHADERFORMAT_SPIRV;
        info.stage = stage;
        info.num_samplers = samplers;
        info.num_uniform_buffers = uniform_buffers;
        info.num_storage_buffers = storage_buffers;
        info.num_storage_textures = storage_textures;

        SDL_GPUShader* shader = SDL_CreateGPUShader(m_device, &info);
        if (!shader) {
            NERROR("Failed to create shader: {}", SDL_GetError());
            return nullptr;
        }
        return shader;
    }

    void Graphics::InitGBuffer() {
        int w, h;
        SDL_GetWindowSize(m_window, &w, &h);
        m_gbuffer.width  = (uint32_t)w;
        m_gbuffer.height = (uint32_t)h;

        // Albedo - R8G8B8A8 is enough for base color
        SDL_GPUTextureCreateInfo albedo_info{};
        albedo_info.type             = SDL_GPU_TEXTURETYPE_2D;
        albedo_info.format           = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        albedo_info.width            = m_gbuffer.width;
        albedo_info.height           = m_gbuffer.height;
        albedo_info.layer_count_or_depth = 1;
        albedo_info.num_levels       = 1;
        albedo_info.usage            = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
        m_gbuffer.albedo = SDL_CreateGPUTexture(m_device, &albedo_info);

        // Normals - need higher precision
        SDL_GPUTextureCreateInfo normals_info = albedo_info;
        normals_info.format  = SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT;
        m_gbuffer.normals = SDL_CreateGPUTexture(m_device, &normals_info);

        // Depth
        SDL_GPUTextureCreateInfo depth_info{};
        depth_info.type              = SDL_GPU_TEXTURETYPE_2D;
        // depth_info.format            = SDL_GPU_TEXTUREFORMAT_32_FLOAT;
        depth_info.format            = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
        depth_info.width             = m_gbuffer.width;
        depth_info.height            = m_gbuffer.height;
        depth_info.layer_count_or_depth = 1;
        depth_info.num_levels        = 1;
        depth_info.usage             = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
        m_gbuffer.depth = SDL_CreateGPUTexture(m_device, &depth_info);

        NINFO("GBuffer initiated {}x{}", m_gbuffer.width, m_gbuffer.height);
    }
    
    void Graphics::DestroyGBuffer() {
        SDL_ReleaseGPUTexture(m_device, m_gbuffer.depth);
        SDL_ReleaseGPUTexture(m_device, m_gbuffer.normals);
        SDL_ReleaseGPUTexture(m_device, m_gbuffer.albedo);
    };

    void Graphics::InitPipelines() {
        // InitPTesting();
        InitPGeometry();
    };

    void Graphics::InitPTesting() {
        auto vert = LoadShader("rect", SDL_GPU_SHADERSTAGE_VERTEX, 0, 1, 0, 0);
        auto frag = LoadShader("rect", SDL_GPU_SHADERSTAGE_FRAGMENT,0, 0, 0, 0);

        // Setup attributes
        SDL_GPUVertexAttribute attrs[3]{};
        attrs[0].location = 0;
        attrs[0].format   = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
        attrs[0].offset   = offsetof(Vertex, x);

        attrs[1].location = 1;
        attrs[1].format   = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
        attrs[1].offset   = offsetof(Vertex, nx);

        attrs[2].location = 2;
        attrs[2].format   = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
        attrs[2].offset   = offsetof(Vertex, u);

        SDL_GPUVertexBufferDescription vdesc{};
        vdesc.slot = 0;
        vdesc.pitch = sizeof(Vertex);
        vdesc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

        SDL_GPUColorTargetDescription ctarget{};
        ctarget.format = SDL_GetGPUSwapchainTextureFormat(m_device, m_window);

        SDL_GPUGraphicsPipelineCreateInfo pipe_info{};
        pipe_info.vertex_shader                                     = vert;
        pipe_info.fragment_shader                                   = frag;
        pipe_info.vertex_input_state.vertex_attributes             = attrs;
        pipe_info.vertex_input_state.num_vertex_attributes         = 2;
        pipe_info.vertex_input_state.vertex_buffer_descriptions    = &vdesc;
        pipe_info.vertex_input_state.num_vertex_buffers            = 1;
        pipe_info.primitive_type                                   = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
        pipe_info.target_info.color_target_descriptions            = &ctarget;
        pipe_info.target_info.num_color_targets                    = 1;

        pipeline = SDL_CreateGPUGraphicsPipeline(m_device, &pipe_info);

        SDL_ReleaseGPUShader(m_device, vert);
        SDL_ReleaseGPUShader(m_device, frag);
        
        SDL_GPUBufferCreateInfo vbuf_info{};
        vbuf_info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
        vbuf_info.size  = sizeof(rect);
        m_vbuf = SDL_CreateGPUBuffer(m_device, &vbuf_info);

        SDL_GPUBufferCreateInfo ibuf_info{};
        ibuf_info.usage = SDL_GPU_BUFFERUSAGE_INDEX;
        ibuf_info.size  = sizeof(indices);
        m_ibuf = SDL_CreateGPUBuffer(m_device, &ibuf_info);


        SDL_GPUTransferBufferCreateInfo tbuf_info{};
        tbuf_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        tbuf_info.size  = sizeof(rect) + sizeof(indices);
        SDL_GPUTransferBuffer* tbuf = SDL_CreateGPUTransferBuffer(m_device, &tbuf_info);

        void* map = SDL_MapGPUTransferBuffer(m_device, tbuf, false);
        memcpy(map, rect, sizeof(rect));
        memcpy((uint8_t*)map + sizeof(rect), indices, sizeof(indices));
        SDL_UnmapGPUTransferBuffer(m_device, tbuf);

        SDL_GPUCommandBuffer* upload_cmd = SDL_AcquireGPUCommandBuffer(m_device);
        SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(upload_cmd);

        SDL_GPUTransferBufferLocation src{};
        SDL_GPUBufferRegion dst{};

        src.transfer_buffer = tbuf;
        src.offset = 0;
        dst.buffer = m_vbuf;
        dst.offset = 0;
        dst.size   = sizeof(rect);
        SDL_UploadToGPUBuffer(copy, &src, &dst, false);

        src.offset = sizeof(rect);
        dst.buffer = m_ibuf;
        dst.size   = sizeof(indices);
        SDL_UploadToGPUBuffer(copy, &src, &dst, false);

        SDL_EndGPUCopyPass(copy);
        SDL_SubmitGPUCommandBuffer(upload_cmd);
        SDL_ReleaseGPUTransferBuffer(m_device, tbuf);
    }

    void Graphics::InitPGeometry() {
        auto vert = LoadShader("g", SDL_GPU_SHADERSTAGE_VERTEX, 0, 1, 0, 0);
        auto frag = LoadShader("g", SDL_GPU_SHADERSTAGE_FRAGMENT,0, 0, 0, 0);

        // Setup attributes
        SDL_GPUVertexAttribute attrs[3]{};
        attrs[0].location = 0;
        attrs[0].format   = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
        attrs[0].offset   = offsetof(Vertex, x);

        attrs[1].location = 1;
        attrs[1].format   = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
        attrs[1].offset   = offsetof(Vertex, nx);

        attrs[2].location = 2;
        attrs[2].format   = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
        attrs[2].offset   = offsetof(Vertex, u);

        SDL_GPUVertexBufferDescription vdesc{};
        vdesc.slot = 0;
        vdesc.pitch = sizeof(Vertex);
        vdesc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

        SDL_GPUColorTargetDescription ctargets[2]{};
        ctargets[0].format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;       // albedo
        ctargets[1].format = SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT;

        SDL_GPUGraphicsPipelineCreateInfo pipe_info{};
        pipe_info.vertex_shader                                     = vert;
        pipe_info.fragment_shader                                   = frag;
        pipe_info.vertex_input_state.vertex_attributes             = attrs;
        pipe_info.vertex_input_state.num_vertex_attributes         = 3;
        pipe_info.vertex_input_state.vertex_buffer_descriptions    = &vdesc;
        pipe_info.vertex_input_state.num_vertex_buffers            = 1;

        pipe_info.primitive_type                                   = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;


        pipe_info.target_info.color_target_descriptions            = ctargets;
        pipe_info.target_info.num_color_targets                    = 2;

        // Depth
        pipe_info.target_info.depth_stencil_format                 = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
        pipe_info.target_info.has_depth_stencil_target              = true;

        m_geometryPipeline = SDL_CreateGPUGraphicsPipeline(m_device, &pipe_info);

        SDL_ReleaseGPUShader(m_device, vert);
        SDL_ReleaseGPUShader(m_device, frag);
        
        SDL_GPUBufferCreateInfo vbuf_info{};
        vbuf_info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
        vbuf_info.size  = sizeof(rect);
        m_vbuf = SDL_CreateGPUBuffer(m_device, &vbuf_info);

        SDL_GPUBufferCreateInfo ibuf_info{};
        ibuf_info.usage = SDL_GPU_BUFFERUSAGE_INDEX;
        ibuf_info.size  = sizeof(indices);
        m_ibuf = SDL_CreateGPUBuffer(m_device, &ibuf_info);


        SDL_GPUTransferBufferCreateInfo tbuf_info{};
        tbuf_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        tbuf_info.size  = sizeof(rect) + sizeof(indices);
        SDL_GPUTransferBuffer* tbuf = SDL_CreateGPUTransferBuffer(m_device, &tbuf_info);

        void* map = SDL_MapGPUTransferBuffer(m_device, tbuf, false);
        memcpy(map, rect, sizeof(rect));
        memcpy((uint8_t*)map + sizeof(rect), indices, sizeof(indices));
        SDL_UnmapGPUTransferBuffer(m_device, tbuf);

        SDL_GPUCommandBuffer* upload_cmd = SDL_AcquireGPUCommandBuffer(m_device);
        SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(upload_cmd);

        SDL_GPUTransferBufferLocation src{};
        SDL_GPUBufferRegion dst{};

        src.transfer_buffer = tbuf;
        src.offset = 0;
        dst.buffer = m_vbuf;
        dst.offset = 0;
        dst.size   = sizeof(rect);
        SDL_UploadToGPUBuffer(copy, &src, &dst, false);

        src.offset = sizeof(rect);
        dst.buffer = m_ibuf;
        dst.size   = sizeof(indices);
        SDL_UploadToGPUBuffer(copy, &src, &dst, false);

        SDL_EndGPUCopyPass(copy);
        SDL_SubmitGPUCommandBuffer(upload_cmd);
        SDL_ReleaseGPUTransferBuffer(m_device, tbuf);
    };

    void Graphics::UpdateCamera(float dt) {
        SDL_Event event;
        // handled in Tick already, so just read mouse/keys directly

        float dx = 0, dy = 0;
        SDL_GetRelativeMouseState(&dx, &dy);

        m_yaw   += dx * m_sensitivity;
        m_pitch = glm::clamp(m_pitch + dy * m_sensitivity, -89.0f, 89.0f); // was minus, now plus

        glm::vec3 dir;
        dir.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
        dir.y = sin(glm::radians(m_pitch));
        dir.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
        dir = glm::normalize(dir);

        glm::vec3 pos   = m_camera.getPos();SDL_SetWindowRelativeMouseMode(m_window, true);
        glm::vec3 right = glm::normalize(glm::cross(dir, {0, 1, 0}));

        const bool* keys = SDL_GetKeyboardState(nullptr);
        if (keys[SDL_SCANCODE_W]) pos += dir   * m_speed * dt;
        if (keys[SDL_SCANCODE_S]) pos -= dir   * m_speed * dt;
        if (keys[SDL_SCANCODE_A]) pos -= right * m_speed * dt;
        if (keys[SDL_SCANCODE_D]) pos += right * m_speed * dt;

        if (keys[SDL_SCANCODE_ESCAPE]) {
            SDL_SetWindowRelativeMouseMode(m_window, false);
        } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT) {
            SDL_SetWindowRelativeMouseMode(m_window, true);
        }

        m_camera.setPos(pos);
        m_camera.setTarget(pos + dir);

        int w, h;
        SDL_GetWindowSize(m_window, &w, &h);
        m_camera.update((float)w / (float)h);
    }
    
};