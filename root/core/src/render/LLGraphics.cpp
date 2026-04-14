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
        InitPLight();
    };

    void Graphics::InitPTesting() {
        // Just removed
    }

    void Graphics::InitPGeometry() {
        auto vert = LoadShader("g", SDL_GPU_SHADERSTAGE_VERTEX, 0, 2, 0, 0);
        auto frag = LoadShader("g", SDL_GPU_SHADERSTAGE_FRAGMENT,2, 0, 0, 0);

        SDL_GPUVertexAttribute attrs[4]{};

        attrs[0].location = 0;
        attrs[0].format   = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
        attrs[0].offset   = offsetof(Vertex, position);

        attrs[1].location = 1;
        attrs[1].format   = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
        attrs[1].offset   = offsetof(Vertex, normal);

        attrs[2].location = 2;
        attrs[2].format   = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
        attrs[2].offset   = offsetof(Vertex, tangent);

        attrs[3].location = 3;
        attrs[3].format   = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
        attrs[3].offset   = offsetof(Vertex, uv);

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
        pipe_info.vertex_input_state.num_vertex_attributes         = 4;

        pipe_info.vertex_input_state.vertex_buffer_descriptions    = &vdesc;
        pipe_info.vertex_input_state.num_vertex_buffers            = 1;
        

        pipe_info.primitive_type                                   = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;


        pipe_info.target_info.color_target_descriptions            = ctargets;
        pipe_info.target_info.num_color_targets                    = 2;

        // Depth
        pipe_info.depth_stencil_state.enable_depth_test            = true;
        pipe_info.depth_stencil_state.enable_depth_write           = true;
        pipe_info.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_GREATER;
        // pipe_info.depth_stencil_state

        pipe_info.target_info.depth_stencil_format                 = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
        pipe_info.target_info.has_depth_stencil_target              = true;
        
        

        pipe_info.rasterizer_state.cull_mode  = SDL_GPU_CULLMODE_BACK;
        pipe_info.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;

        m_geometryPipeline = SDL_CreateGPUGraphicsPipeline(m_device, &pipe_info);

        SDL_ReleaseGPUShader(m_device, vert);
        SDL_ReleaseGPUShader(m_device, frag);
    };

    void Graphics::InitPLight() {
        auto vert = LoadShader("l", SDL_GPU_SHADERSTAGE_VERTEX, 0, 0, 0, 0);
        auto frag = LoadShader("l", SDL_GPU_SHADERSTAGE_FRAGMENT, 2, 1, 0, 0);
        //                                                samplers^ ^uniform

        SDL_GPUColorTargetDescription ctarget{};
        ctarget.format = SDL_GetGPUSwapchainTextureFormat(m_device, m_window);

        SDL_GPUGraphicsPipelineCreateInfo pipe_info{};
        pipe_info.vertex_shader   = vert;
        pipe_info.fragment_shader = frag;
        pipe_info.primitive_type  = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

        // No vertex attributes, no vertex buffers
        pipe_info.vertex_input_state.num_vertex_attributes = 0;
        pipe_info.vertex_input_state.num_vertex_buffers    = 0;

        // One color target, no depth
        pipe_info.target_info.color_target_descriptions = &ctarget;
        pipe_info.target_info.num_color_targets         = 1;
        pipe_info.target_info.has_depth_stencil_target  = false;

        // No culling for fullscreen triangle
        pipe_info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;

        m_lightPipeline = SDL_CreateGPUGraphicsPipeline(m_device, &pipe_info);

        SDL_ReleaseGPUShader(m_device, vert);
        SDL_ReleaseGPUShader(m_device, frag);
    }

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

        glm::vec3 pos   = m_camera.getPos();
        glm::vec3 right = glm::normalize(glm::cross(dir, {0, 1, 0}));

        // On shift be much faster
        float m_speed = 3.0f;
        if (SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_LSHIFT]) m_speed = 10.0f;

        const bool* keys = SDL_GetKeyboardState(nullptr);
        if (keys[SDL_SCANCODE_W]) pos += dir   * m_speed * dt;
        if (keys[SDL_SCANCODE_S]) pos -= dir   * m_speed * dt;
        if (keys[SDL_SCANCODE_A]) pos -= right * m_speed * dt;
        if (keys[SDL_SCANCODE_D]) pos += right * m_speed * dt;



        if (keys[SDL_SCANCODE_ESCAPE]) {
            SDL_SetWindowRelativeMouseMode(m_window, false);
        }

        m_camera.setPos(pos);
        m_camera.setTarget(pos + dir);

        int w, h;
        SDL_GetWindowSize(m_window, &w, &h);
        m_camera.update((float)w / (float)h);
    }

    Object* Graphics::AddObject() {
        auto& ptr = m_objects.emplace_back(std::make_unique<Object>(m_device, m_context.GetSamplers()));
        ptr->Init();
        return ptr.get();
    }

    void Graphics::RemoveObject(Object* obj) {
        std::erase_if(m_objects, [obj](const auto& ptr) {
            return ptr.get() == obj;
        });
    }
    
};