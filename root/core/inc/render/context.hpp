#pragma once

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_video.h>

#define VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES 49

namespace N::Graphics {

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

    struct Samplers {
        SDL_GPUSampler* linear  = nullptr;
        SDL_GPUSampler* nearest = nullptr;
        SDL_GPUSampler* shadow  = nullptr;
    };

    class Context {
        public:
            Context() {};
            ~Context() {}

        public:
            SDL_GPUDevice* GetDevice() const { return m_device; }
            SDL_Window* GetWindow() const { return m_window; }
            const Samplers& GetSamplers() const { return m_samplers; }

            void SetVsync(bool vsync) {
                SDL_GPUPresentMode mode = vsync
                    ? SDL_GPU_PRESENTMODE_VSYNC
                    : SDL_GPU_PRESENTMODE_MAILBOX;
                
                if(!SDL_WindowSupportsGPUPresentMode(m_device, m_window, mode)) {
                    mode = SDL_GPU_PRESENTMODE_IMMEDIATE;
                }

                SDL_SetGPUSwapchainParameters(m_device, m_window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, mode);
            };

        public:
            bool Init();
                bool _InitWindow();
                bool _InitDevice();
                bool _InitSamplers();

            bool Shutdown();
                bool _ShutdownSamplers();
                bool _ShutdownDevice();
                bool _ShutdownWindow();

        private:
            SDL_GPUDevice* m_device = nullptr;
            Samplers m_samplers;
            SDL_Window* m_window = nullptr;
    };
};