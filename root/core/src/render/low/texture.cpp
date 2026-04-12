#include "render/texture.hpp"
#include "log/log.hpp"
#include "stb/stb_image.h"

namespace N::Graphics {
    bool Texture::LoadFromCgltf(SDL_GPUDevice* device, cgltf_image* image) {
        int w, h, channels;
        stbi_uc* pixels = nullptr;

        if (image->buffer_view) {
            // embedded in glb
            void* data = (char*)image->buffer_view->buffer->data + image->buffer_view->offset;
            size_t size = image->buffer_view->size;
            pixels = stbi_load_from_memory((stbi_uc*)data, (int)size, &w, &h, &channels, 4);
        } else if (image->uri) {
            // external file
            pixels = stbi_load(image->uri, &w, &h, &channels, 4);
        }

        if (!pixels) {
            LOG_ERROR("Failed to load texture: {}", image->uri ? image->uri : "embedded");
            return false;
        }

        width  = (uint32_t)w;
        height = (uint32_t)h;

        // create SDL GPU texture
        SDL_GPUTextureCreateInfo tex_info{};
        tex_info.type   = SDL_GPU_TEXTURETYPE_2D;
        tex_info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        tex_info.usage  = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        tex_info.width  = width;
        tex_info.height = height;
        tex_info.layer_count_or_depth = 1;
        tex_info.num_levels = 1;

        m_texture = SDL_CreateGPUTexture(device, &tex_info);

        // upload pixels
        SDL_GPUTransferBufferCreateInfo tbuf_info{};
        tbuf_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        tbuf_info.size  = width * height * 4;
        SDL_GPUTransferBuffer* tbuf = SDL_CreateGPUTransferBuffer(device, &tbuf_info);

        void* map = SDL_MapGPUTransferBuffer(device, tbuf, false);
        memcpy(map, pixels, width * height * 4);
        SDL_UnmapGPUTransferBuffer(device, tbuf);
        stbi_image_free(pixels);

        SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
        SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd);

        SDL_GPUTextureTransferInfo src{};
        src.transfer_buffer = tbuf;
        src.offset = 0;

        SDL_GPUTextureRegion dst{};
        dst.texture = m_texture;
        dst.w = width;
        dst.h = height;
        dst.d = 1;

        SDL_UploadToGPUTexture(copy, &src, &dst, false);
        SDL_EndGPUCopyPass(copy);
        SDL_SubmitGPUCommandBuffer(cmd);
        SDL_ReleaseGPUTransferBuffer(device, tbuf);

        LOG_INFO("Texture loaded");

        return true;
    }
};