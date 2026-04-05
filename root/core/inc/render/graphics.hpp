#pragma once

#include "log/log.hpp"
#include "render/camera.hpp"
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_video.h>
#include <cstdint>

namespace N::Graphics {
    struct Vertex {
        float x, y, z;
        float nx, ny, nz;
        float u, v;
    };

    static const Vertex rect[] = {
        { -0.5f,  0.5f, 0.0f,  0, 0, -1,  0, 0 },
        {  0.5f,  0.5f, 0.0f,  0, 0, -1,  1, 0 },
        {  0.5f, -0.5f, 0.0f,  0, 0, -1,  1, 1 },
        { -0.5f, -0.5f, 0.0f,  0, 0, -1,  0, 1 },
    };

    static const uint16_t indices[] = {0, 1, 2, 0, 2, 3};

    constexpr std::string SHADER_PATH = "assets/shaders/";



    class Graphics {
        public:
            Graphics() {};
            ~Graphics() {};

        public:
            void Init();

            bool Tick();

            void Shutdown();


            // Swapchain related
            void SetVsync(bool vsync);
        private:
            SDL_GPUShader* LoadShader(const char* file, SDL_GPUShaderStage stage, Uint32 samplers, Uint32 uniform_buffers, Uint32 storage_buffers, Uint32 storage_textures);
            // Pipelines
            void InitPipelines(); // Functions which load pipelines will have a prefix of InitP (ex. InitPTesting) 
                void InitPTesting();
                void InitPGeometry(); // GBuffer

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
                uint32_t width  = 0;
                uint32_t height = 0;
            } m_gbuffer;

            SDL_GPUGraphicsPipeline *pipeline;
            SDL_GPUGraphicsPipeline *m_geometryPipeline;

            Camera m_camera;

            SDL_GPUBuffer* m_vbuf;
            SDL_GPUBuffer* m_ibuf;


            private:
                float m_yaw   = -90.0f;
                float m_pitch = 0.0f;
                bool m_firstMouse = true;
                float m_lastX = 400, m_lastY = 300;
                float m_speed = 3.0f;
                float m_sensitivity = 0.1f;
                float m_lastTime = 0.0f;
        };
}