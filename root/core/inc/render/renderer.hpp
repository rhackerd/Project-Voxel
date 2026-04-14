#pragma once

#include "render/context.hpp"
#include "render/obj.hpp"
#include <span>
namespace N::Graphics {
    
    class Renderer {
        public:
            Renderer(Context& ctx) : m_ctx(ctx) {};
            ~Renderer();

        public:
            bool Init();
                bool _InitGBuffer();
                bool _InitPipelines();
                    bool _InitPGeometry();
                    bool _InitPLight();        

            // TODO: Ordering of rendering (Front to Back) - later optimization
            // TODO: Later add rather RenderObject than just 'Model'
            void Draw(std::span<Model> objects);
                void _DrawGBuffer();
                    void _DrawModel(Model& model);
                void _DrawLight();

            bool Shutdown();
                bool _DestroyGBuffer();
                bool _DestroyPipelines();
                    bool _DestroyPGeometry();
                    bool _DestroyPLight();

        private:
            Context& m_ctx;
    };
};