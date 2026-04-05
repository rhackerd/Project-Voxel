#include "render/graphics.hpp"
#include "log/log.hpp"
#include "render/camera.hpp"
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>
#include <cstddef>

namespace N::Graphics {
    void Graphics::Init() {
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            NERROR("Failed to initiate SDL: {}", SDL_GetError());
            return;
        }

        // --- Window ----

        m_window = SDL_CreateWindow("Nova Engine", 800, 600, SDL_WINDOW_RESIZABLE);
        if (!m_window) {
            NERROR("Failed to create window: {}", SDL_GetError());
            return;
        }

        // --- GPU ----

        SDL_GPUVulkanOptions vk_options{};
        vk_options.vulkan_api_version = (1 << 22) | (3 << 12);

        SDL_PropertiesID _props = SDL_CreateProperties();
        SDL_SetPointerProperty(_props, SDL_PROP_GPU_DEVICE_CREATE_VULKAN_OPTIONS_POINTER, &vk_options);
        SDL_SetBooleanProperty(_props, SDL_PROP_GPU_DEVICE_CREATE_DEBUGMODE_BOOLEAN, true);
        SDL_SetBooleanProperty(_props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_SPIRV_BOOLEAN, true);

        m_device = SDL_CreateGPUDeviceWithProperties(_props);
        SDL_DestroyProperties(_props);

        if (!m_device) {
            NERROR("Failed to create GPU Device: {}", SDL_GetError());
            return;
        }

        NINFO("Create GPU Device");
        
        // ---- GPU INFO ----

        NINFO("Getting device properties: ");
        SDL_PropertiesID props = SDL_GetGPUDeviceProperties(m_device);
        const char* gpu_name = SDL_GetStringProperty(props, SDL_PROP_GPU_DEVICE_NAME_STRING, "unknown device");
    
        NINFO("   -> Device: {} ({})", gpu_name, SDL_GetGPUDeviceDriver(m_device));

        // ---- GPU <=> Windiw -----
        SDL_ClaimWindowForGPUDevice(m_device, m_window);
        // running = true;

    
        InitGBuffer();

        NINFO("Making Graphics Pipeline");
        InitPipelines();

        m_camera.setPos({0, 0, -3.0f});
        m_camera.setTarget({0, 0, 0});

        SDL_SetWindowRelativeMouseMode(m_window, true);
    }

    bool Graphics::Tick() {

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                return false;
            }else if(event.type == SDL_EVENT_WINDOW_RESIZED) {
                m_camera.update((float)event.window.data1 / (float)event.window.data2);
                // Resize gBuffer
                DestroyGBuffer();
                InitGBuffer();
            }
        }
        float now = (float)SDL_GetTicks() / 1000.0f;
        float dt  = now - m_lastTime;
        m_lastTime = now;


        UpdateCamera(dt);

        SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(m_device);

        SDL_GPUTexture* swapchain_tex;
        SDL_AcquireGPUSwapchainTexture(cmd, m_window, &swapchain_tex, nullptr, nullptr);

        if (swapchain_tex) {
            // --- Geometry Pass ---
            SDL_GPUColorTargetInfo geo_targets[2]{};
            geo_targets[0].texture     = m_gbuffer.albedo;
            geo_targets[0].clear_color = {0, 0, 0, 1};
            geo_targets[0].load_op     = SDL_GPU_LOADOP_CLEAR;
            geo_targets[0].store_op    = SDL_GPU_STOREOP_STORE;

            geo_targets[1].texture     = m_gbuffer.normals;
            geo_targets[1].clear_color = {0, 0, 0, 1};
            geo_targets[1].load_op     = SDL_GPU_LOADOP_CLEAR;
            geo_targets[1].store_op    = SDL_GPU_STOREOP_STORE;

            SDL_GPUDepthStencilTargetInfo depth_target{};
            depth_target.texture         = m_gbuffer.depth;
            depth_target.clear_depth     = 1.0f;
            depth_target.load_op         = SDL_GPU_LOADOP_CLEAR;
            depth_target.store_op        = SDL_GPU_STOREOP_STORE;

            SDL_GPURenderPass* geo_pass = SDL_BeginGPURenderPass(cmd, geo_targets, 2, &depth_target);

            SDL_BindGPUGraphicsPipeline(geo_pass, m_geometryPipeline);

            // push camera
            CameraUBO ubo = m_camera.getUBO();
            SDL_PushGPUVertexUniformData(cmd, 0, &ubo, sizeof(CameraUBO));

            SDL_GPUBufferBinding vbind{.buffer = m_vbuf, .offset = 0};
            SDL_BindGPUVertexBuffers(geo_pass, 0, &vbind, 1);

            SDL_GPUBufferBinding ibind{.buffer = m_ibuf, .offset = 0};
            SDL_BindGPUIndexBuffer(geo_pass, &ibind, SDL_GPU_INDEXELEMENTSIZE_16BIT);


            SDL_DrawGPUIndexedPrimitives(geo_pass, 6, 1, 0, 0, 0);
            SDL_EndGPURenderPass(geo_pass);

            // --- Debug blit albedo → swapchain ---
            SDL_GPUBlitInfo blit{};
            blit.source.texture      = m_gbuffer.albedo;
            blit.source.w            = m_gbuffer.width;
            blit.source.h            = m_gbuffer.height;
            blit.destination.texture = swapchain_tex;
            blit.destination.w       = m_gbuffer.width;
            blit.destination.h       = m_gbuffer.height;
            blit.load_op             = SDL_GPU_LOADOP_CLEAR;
            SDL_BlitGPUTexture(cmd, &blit);
        }

        SDL_SubmitGPUCommandBuffer(cmd);

        return true;
    }

    void Graphics::Shutdown() {

        DestroyGBuffer();

        // Buffers
        SDL_ReleaseGPUBuffer(m_device, m_vbuf);
        SDL_ReleaseGPUBuffer(m_device, m_ibuf);

        

        SDL_ReleaseGPUGraphicsPipeline(m_device, m_geometryPipeline);
        SDL_ReleaseWindowFromGPUDevice(m_device, m_window);

        SDL_DestroyWindow(m_window);
        NINFO("Destroy Window");

        

        SDL_DestroyGPUDevice(m_device);
        NINFO("Destroy GPU Device");
        NINFO("Goodbye from Graphics");
    }

    void Graphics::SetVsync(bool vsync) {
        SDL_GPUPresentMode mode = vsync
            ? SDL_GPU_PRESENTMODE_VSYNC
            : SDL_GPU_PRESENTMODE_MAILBOX;
        
        if(!SDL_WindowSupportsGPUPresentMode(m_device, m_window, mode)) {
            mode = SDL_GPU_PRESENTMODE_IMMEDIATE;
        }

        SDL_SetGPUSwapchainParameters(m_device, m_window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, mode);
    }
};