#include "render/graphics.hpp"
#include "log/log.hpp"
#include "render/camera.hpp"
#include "render/obj.hpp"
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_version.h>
#include <SDL3/SDL_video.h>
#include <cstddef>

struct VkPhysicalDeviceVulkan11Features {
    uint32_t sType;
    void* pNext;
    uint32_t storageBuffer16BitAccess;
    uint32_t uniformAndStorageBuffer16BitAccess;
    uint32_t storagePushConstant16;
    uint32_t storageInputOutput16;
    uint32_t multiview;
    uint32_t multiviewGeometryShader;
    uint32_t multiviewTessellationShader;
    uint32_t variablePointersStorageBuffer;
    uint32_t variablePointers;
    uint32_t protectedMemory;
    uint32_t samplerYcbcrConversion;
    uint32_t shaderDrawParameters;
};

#define VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES 49

namespace N::Graphics {
    void Graphics::Init() {
        // Print sdl3 version
        NINFO("SDL3 Version: {}", SDL_GetVersion());
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

        VkPhysicalDeviceVulkan11Features vk11_features{};
        vk11_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
        vk11_features.shaderDrawParameters = 1;
        vk_options.feature_list = &vk11_features;

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
        m_camera.setFOV(90.0f);

        SDL_SetWindowRelativeMouseMode(m_window, true);

        InitSamplers();
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
            } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT) {
                SDL_SetWindowRelativeMouseMode(m_window, true);
                // printf("Relative mouse mode on\n");
                // SDL_SetWindowRelativeMouseMode(m_window, true);
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
            depth_target.clear_depth     = 0.0f;
            depth_target.load_op         = SDL_GPU_LOADOP_CLEAR;
            depth_target.store_op        = SDL_GPU_STOREOP_STORE;
            // depth_target.clear_depth = 0.1f;
            // depth_target.cycle           = true;

            //
            // --- GBuffer Pass ---
            //

            SDL_GPURenderPass* geo_pass = SDL_BeginGPURenderPass(cmd, geo_targets, 2, &depth_target);

            SDL_BindGPUGraphicsPipeline(geo_pass, m_geometryPipeline);

            // push camera
            CameraUBO ubo = m_camera.getUBO();
            SDL_PushGPUVertexUniformData(cmd, 0, &ubo, sizeof(CameraUBO));

            for (auto &ptr : m_objects) {
                ptr->Draw(geo_pass, cmd);
            }
            

            // Now draw the box
            SDL_GPUBufferBinding vbind{.buffer = m_vbuf, .offset = 0};
            SDL_BindGPUVertexBuffers(geo_pass, 0, &vbind, 1);

            SDL_GPUBufferBinding ibind{.buffer = m_ibuf, .offset = 0};
            SDL_BindGPUIndexBuffer(geo_pass, &ibind, SDL_GPU_INDEXELEMENTSIZE_32BIT);

            // SDL_DrawGPUIndexedPrimitives(geo_pass, 36, 1, 0, 0, 0);

            SDL_EndGPURenderPass(geo_pass);

            //
            // --- Light pass ---
            //

            SDL_GPUColorTargetInfo light_target{};
            light_target.texture = swapchain_tex;
            light_target.load_op = SDL_GPU_LOADOP_CLEAR;
            light_target.store_op = SDL_GPU_STOREOP_STORE;
            light_target.clear_color = {0.0f, 0.0f, 0.0f, 1.0f};

            SDL_GPURenderPass* light_pass = SDL_BeginGPURenderPass(cmd, &light_target, 1, NULL);
            SDL_BindGPUGraphicsPipeline(light_pass, m_lightPipeline);

            SDL_GPUTextureSamplerBinding albedoBind = {m_gbuffer.albedo, m_samplers.linear};
            SDL_BindGPUFragmentSamplers(light_pass, 0, &albedoBind, 1);

            SDL_GPUTextureSamplerBinding normalBind = {m_gbuffer.normals, m_samplers.linear};
            SDL_BindGPUFragmentSamplers(light_pass, 1, &normalBind, 1);

            DirLight light{};
            light.direction = glm::vec3(0.5f, -1.0f, 0.3f);
            light.intensity = 0.5f;
            light.color = glm::vec3(1.0f);
            SDL_PushGPUFragmentUniformData(cmd, 0, &light, sizeof(DirLight));

            SDL_DrawGPUPrimitives(light_pass, 3, 1, 0, 0);

            SDL_EndGPURenderPass(light_pass);

        }

        SDL_SubmitGPUCommandBuffer(cmd);

        return true;
    }

    void Graphics::Shutdown() {



        // First destroy the Textures
        DestroyGBuffer();



        for (auto &ptr : m_objects) {
            ptr->Shutdown();
            
        }

        SDL_ReleaseGPUSampler(m_device, m_samplers.linear);
        SDL_ReleaseGPUSampler(m_device, m_samplers.nearest);
        SDL_ReleaseGPUSampler(m_device, m_samplers.shadow);

        // Buffers
        SDL_ReleaseGPUBuffer(m_device, m_vbuf);
        SDL_ReleaseGPUBuffer(m_device, m_ibuf);

        

        SDL_ReleaseGPUGraphicsPipeline(m_device, m_geometryPipeline);
        SDL_ReleaseGPUGraphicsPipeline(m_device, m_lightPipeline);
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