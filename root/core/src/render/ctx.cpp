#include "log/log.hpp"
#include "render/context.hpp"
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_video.h>

namespace N::Graphics {
    bool Context::Init() {
        if (!_InitWindow()) return false;
        if (!_InitDevice()) return false;
        if (!_InitSamplers()) return false;
        
        SDL_ClaimWindowForGPUDevice(m_device, m_window);

        return true;
    };

    bool Context::Shutdown() {
        SDL_ReleaseWindowFromGPUDevice(m_device, m_window);
        if (!_ShutdownSamplers()) return false;
        if (!_ShutdownDevice()) return false;
        if (!_ShutdownWindow()) return false;
        return true;
    };

    bool Context::_InitWindow() {
        m_window = SDL_CreateWindow("Nova Engine", 800, 600, 0);
        if (!m_window) {
            LOG_ERROR("Failed to create window: {}", SDL_GetError());
            return false;
        };
        return true;
    };

    bool Context::_InitDevice() {
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
            LOG_ERROR("Failed to create GPU Device: {}", SDL_GetError());
            return false;
        }

        return true;
    };

    bool Context::_InitSamplers() {
        SDL_GPUSamplerCreateInfo info{};

        info.min_filter     = SDL_GPU_FILTER_LINEAR;
        info.mag_filter     = SDL_GPU_FILTER_LINEAR;
        info.mipmap_mode    = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
        info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
        info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
        m_samplers.linear   = SDL_CreateGPUSampler(m_device, &info);

        // Nearest repeat — pixel art / UI
        info.min_filter     = SDL_GPU_FILTER_NEAREST;
        info.mag_filter     = SDL_GPU_FILTER_NEAREST;
        info.mipmap_mode    = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
        m_samplers.nearest  = SDL_CreateGPUSampler(m_device, &info);

        // Shadow — clamp to border, for shadow maps later
        info.min_filter     = SDL_GPU_FILTER_LINEAR;
        info.mag_filter     = SDL_GPU_FILTER_LINEAR;
        info.mipmap_mode    = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
        info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        m_samplers.shadow   = SDL_CreateGPUSampler(m_device, &info);

        if (!m_samplers.linear || !m_samplers.nearest || !m_samplers.shadow) {
            LOG_ERROR("Failed to create samplers: {}", SDL_GetError());
            return false;
        }
        return true;
    };

    bool Context::_ShutdownDevice() {
        SDL_DestroyGPUDevice(m_device);
        m_device = nullptr;

        return true;
    };

    bool Context::_ShutdownWindow() {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
        return true;
    };

    bool Context::_ShutdownSamplers() {
        SDL_ReleaseGPUSampler(m_device, m_samplers.linear);
        SDL_ReleaseGPUSampler(m_device, m_samplers.nearest);
        SDL_ReleaseGPUSampler(m_device, m_samplers.shadow);
        return true;
    };
};