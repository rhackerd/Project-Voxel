#pragma once

#include "log/log.hpp"
#include "render/camera.hpp"
#include "render/core.hpp"
#include "render/obj.hpp"
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_video.h>
#include <cstdint>
#include <vector>

namespace N::Graphics {

    struct DirLight {
        glm::vec3 direction;
        float intensity;
        glm::vec3 color;
        float _pad;
    };

    static const Vertex box[] = {
        // Front (0,0,-1)
        { { -0.5f,  0.5f, -0.5f }, { 0, 0, -1 }, { 1, 0, 0, 1 }, { 0, 0 } },
        { {  0.5f,  0.5f, -0.5f }, { 0, 0, -1 }, { 1, 0, 0, 1 }, { 1, 0 } },
        { {  0.5f, -0.5f, -0.5f }, { 0, 0, -1 }, { 1, 0, 0, 1 }, { 1, 1 } },
        { { -0.5f, -0.5f, -0.5f }, { 0, 0, -1 }, { 1, 0, 0, 1 }, { 0, 1 } },

        // Back (0,0,1)
        { {  0.5f,  0.5f,  0.5f }, { 0, 0, 1 }, { -1, 0, 0, 1 }, { 0, 0 } },
        { { -0.5f,  0.5f,  0.5f }, { 0, 0, 1 }, { -1, 0, 0, 1 }, { 1, 0 } },
        { { -0.5f, -0.5f,  0.5f }, { 0, 0, 1 }, { -1, 0, 0, 1 }, { 1, 1 } },
        { {  0.5f, -0.5f,  0.5f }, { 0, 0, 1 }, { -1, 0, 0, 1 }, { 0, 1 } },

        // Left (-1,0,0)
        { { -0.5f,  0.5f,  0.5f }, { -1, 0, 0 }, { 0, 0, -1, 1 }, { 0, 0 } },
        { { -0.5f,  0.5f, -0.5f }, { -1, 0, 0 }, { 0, 0, -1, 1 }, { 1, 0 } },
        { { -0.5f, -0.5f, -0.5f }, { -1, 0, 0 }, { 0, 0, -1, 1 }, { 1, 1 } },
        { { -0.5f, -0.5f,  0.5f }, { -1, 0, 0 }, { 0, 0, -1, 1 }, { 0, 1 } },

        // Right (1,0,0)
        { {  0.5f,  0.5f, -0.5f }, { 1, 0, 0 }, { 0, 0, 1, 1 }, { 0, 0 } },
        { {  0.5f,  0.5f,  0.5f }, { 1, 0, 0 }, { 0, 0, 1, 1 }, { 1, 0 } },
        { {  0.5f, -0.5f,  0.5f }, { 1, 0, 0 }, { 0, 0, 1, 1 }, { 1, 1 } },
        { {  0.5f, -0.5f, -0.5f }, { 1, 0, 0 }, { 0, 0, 1, 1 }, { 0, 1 } },

        // Top (0,1,0)
        { { -0.5f,  0.5f,  0.5f }, { 0, 1, 0 }, { 1, 0, 0, 1 }, { 0, 0 } },
        { {  0.5f,  0.5f,  0.5f }, { 0, 1, 0 }, { 1, 0, 0, 1 }, { 1, 0 } },
        { {  0.5f,  0.5f, -0.5f }, { 0, 1, 0 }, { 1, 0, 0, 1 }, { 1, 1 } },
        { { -0.5f,  0.5f, -0.5f }, { 0, 1, 0 }, { 1, 0, 0, 1 }, { 0, 1 } },

        // Bottom (0,-1,0)
        { { -0.5f, -0.5f, -0.5f }, { 0, -1, 0 }, { 1, 0, 0, 1 }, { 0, 0 } },
        { {  0.5f, -0.5f, -0.5f }, { 0, -1, 0 }, { 1, 0, 0, 1 }, { 1, 0 } },
        { {  0.5f, -0.5f,  0.5f }, { 0, -1, 0 }, { 1, 0, 0, 1 }, { 1, 1 } },
        { { -0.5f, -0.5f,  0.5f }, { 0, -1, 0 }, { 1, 0, 0, 1 }, { 0, 1 } },
    };

    static const uint32_t box_indices[] = {
        0,  1,  2,  0,  2,  3,  // front
        4,  5,  6,  4,  6,  7,  // back
        8,  9, 10,  8, 10, 11,  // left
        12, 13, 14, 12, 14, 15,  // right
        16, 17, 18, 16, 18, 19,  // top
        20, 21, 22, 20, 22, 23,  // bottom
    };

    constexpr std::string SHADER_PATH = "assets/shaders/";



    class Graphics {
        public:
            Graphics() {
                Init();
            };
            ~Graphics() {
                Shutdown();
            };

        public:
            void Init();
                void InitSamplers();

            bool Tick();

            void Shutdown();


            // Swapchain related
            void SetVsync(bool vsync);

            // Object related functions
            Object* AddObject();
            void RemoveObject(Object* object);

        private:
            SDL_GPUShader* LoadShader(const char* file, SDL_GPUShaderStage stage, Uint32 samplers, Uint32 uniform_buffers, Uint32 storage_buffers, Uint32 storage_textures);
            // Pipelines
            void InitPipelines(); // Functions which load pipelines will have a prefix of InitP (ex. InitPTesting) 
                void InitPTesting();
                void InitPGeometry(); // GBuffer
                void InitPLight();

            void InitGBuffer();
            void DestroyGBuffer();

            void UpdateCamera(float dt);

        private:
            SDL_Window* m_window = nullptr;
            SDL_GPUDevice* m_device = nullptr;
            NOVA_LOG_DEF("Graphics");
            // bool running = false;

            struct GBuffer {
                SDL_GPUTexture* depth = nullptr;
                SDL_GPUTexture* normals  = nullptr;
                SDL_GPUTexture* albedo   = nullptr;
                SDL_GPUTexture* emissive = nullptr;
                uint32_t width  = 0;
                uint32_t height = 0;
            } m_gbuffer;

            SDL_GPUGraphicsPipeline *pipeline;
            SDL_GPUGraphicsPipeline *m_geometryPipeline;
            SDL_GPUGraphicsPipeline *m_lightPipeline;

            Camera m_camera;

            SDL_GPUBuffer* m_vbuf;
            SDL_GPUBuffer* m_ibuf;

        private:
            std::vector<std::unique_ptr<Object>> m_objects;
            Samplers m_samplers;

            private: // Remove later
                float m_yaw   = -90.0f;
                float m_pitch = 0.0f;
                bool m_firstMouse = true;
                float m_lastX = 400, m_lastY = 300;
                float m_speed = 0.1f;
                float m_sensitivity = 0.1f;
                float m_lastTime = 0.0f;
        };
}