#pragma once
#include "types.h"
#include <SDL3/SDL_gpu.h>
namespace N::Graphics {
    class Texture {
        public:

            void Init(SDL_GPUDevice* device, u32 w, u32 h);

            
            bool LoadFromFile(SDL_GPUDevice* device, const char* path);
                void Upload(SDL_GPUDevice* device, const void* data);


            void Destroy(SDL_GPUDevice* device) {
                if (m_texture) {
                    SDL_ReleaseGPUTexture(device, m_texture);
                    m_texture = nullptr;
                }
            }

            SDL_GPUTexture* Get() const {return m_texture;}
            bool IsValid() const { return m_texture != nullptr;}

            uint32_t GetWidth() const {return width;}
            uint32_t GetHeight() const {return height;}

            

        private:
            uint32_t width = 0;
            uint32_t height = 0;

        private:
            SDL_GPUTexture* m_texture = nullptr;
    };
};