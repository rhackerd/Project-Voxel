#include "render/texture.hpp"
#include "log/log.hpp"
#include "stb/stb_image.h"

namespace N::Graphics {

    void Texture::Init(SDL_GPUDevice* device, u32 w, u32 h) {
        width   = w;
        height  = h;

        SDL_GPUTextureCreateInfo info{};
        info.type = SDL_GPU_TEXTURETYPE_2D;
        info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        info.width = width;
        info.height = height;
        info.layer_count_or_depth = 1;
        info.num_levels = 1;

        m_texture = SDL_CreateGPUTexture(device, &info);
    };

    void Texture::Upload(SDL_GPUDevice* device, const void* data) {
        u32 size = width*height*4;

        SDL_GPUTransferBufferCreateInfo tbuf_info{};
        tbuf_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        tbuf_info.size = size;
        SDL_GPUTransferBuffer* tbuf = SDL_CreateGPUTransferBuffer(device, &tbuf_info);

        void* map = SDL_MapGPUTransferBuffer(device, tbuf, false);
        memcpy(map, data, size);
        SDL_UnmapGPUTransferBuffer(device, tbuf);

        SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
        SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd);

        SDL_GPUTextureTransferInfo src{};
        src.transfer_buffer = tbuf;

        SDL_GPUTextureRegion dst{};
        dst.texture = m_texture;
        dst.w = width;
        dst.h = height;
        dst.d = 1;

        SDL_UploadToGPUTexture(copy, &src, &dst, false);
        SDL_EndGPUCopyPass(copy);
        SDL_SubmitGPUCommandBuffer(cmd);
        SDL_ReleaseGPUTransferBuffer(device, tbuf);
    };

    bool Texture::LoadFromFile(SDL_GPUDevice* device, const char* path) {
        int w, h, chanels;
        stbi_uc* pixels = stbi_load(path, &w, &h, &chanels, 4);
        if (!pixels) {
            LOG_ERROR("Failed to load texture: {}", path);
            return false;
        };

        Init(device, w, h);
        Upload(device, pixels);
        stbi_image_free(pixels);

        LOG_INFO("Texture loaded {}", path);
        return true;
    };
};