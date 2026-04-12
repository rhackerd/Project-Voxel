#pragma once

#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <SDL3/SDL_gpu.h>
#include <string>
#include <utility>
#include <vector>
#include "render/cgltf.h"
#include "mesh.hpp"
#include "render/core.hpp"
#include "render/texture.hpp"


namespace N::Graphics {

    struct Vertex {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec4 tangent; // xyz = tangent, w = bitangent sign (+1 or -1)
        glm::vec2 uv;
    };

    struct PBRTex {
        Texture albedo;
        Texture normal;
        Texture metalRough;
        Texture emissive;
    };

    struct ObjPC {
        glm::mat4 model;
        glm::mat4 normalMat;
    };

    struct Model {
        SDL_GPUBuffer* vbuf = nullptr;
        SDL_GPUBuffer* ibuf = nullptr;
        size_t indexCount = 0;
    };


    struct Mesh {
        uint32_t indexOffset = 0;
        uint32_t indexCount = 0;
        uint32_t vertexOffset = 0;
        PBRTex textures{};
        glm::mat4 nodeTransform = glm::mat4(1.0f);
    };

    class Object {
    public:
        Object(SDL_GPUDevice* device, Samplers samplers) {
            this->m_device = device;
            this->m_samplers = samplers;
        };
        ~Object() {};

    void Init();
        void CreateModelBuffers(size_t vert_num, size_t ind_num);

    void Draw(SDL_GPURenderPass* pass, SDL_GPUCommandBuffer* cmd);

    void Shutdown();
        void DestroyModelBuffers();

    void LoadModel(const char* path);
        void GatherGeometry(cgltf_data* data, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);
        bool LoadPBRTextures(cgltf_data* data);
        void UploadToGPU(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);
        void LoadMesh(cgltf_node* node, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);
        void GenerateTangents(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);

    public:

        // TODO: Later move these into a .cpp file
        glm::mat4 getLocalTransform() const {
            glm::mat4 t = glm::translate(glm::mat4(1.0f), m_position);
            glm::mat4 r = glm::rotate(glm::mat4(1.0f), glm::radians(m_rotation.y), {0,1,0});
                        r = glm::rotate(r, glm::radians(m_rotation.x), {1,0,0});
                        r = glm::rotate(r, glm::radians(m_rotation.z), {0,0,1});
            glm::mat4 s = glm::scale(glm::mat4(1.0f), m_scale);
            return t * r * s;
        }

        ObjPC getMeshPC(const Mesh& mesh) const {
            glm::mat4 model = getLocalTransform() * mesh.nodeTransform;
            glm::mat4 normalMat = glm::mat4(glm::transpose(glm::inverse(glm::mat3(model))));
            return { model, normalMat };
        }

        // Moving setters and getters
        void setPosition(glm::vec3 pos) { m_position = pos; }
        void setRotation(glm::vec3 rot) { m_rotation = rot; }
        void setScale(glm::vec3 sca) { m_scale = sca; }

        glm::vec3 getPosition() { return m_position; }
        glm::vec3 getRotation() { return m_rotation; }
        glm::vec3 getScale() { return m_scale; }

        void move(float x, float y, float z) { m_position += glm::vec3(x,y,z); }
        void rotate(float x, float y, float z) { m_rotation += glm::vec3(x,y,z); }
        void scale(float x, float y, float z) { m_scale += glm::vec3(x,y,z); }

        void InitFallback();

    private:
        SDL_GPUDevice* m_device = nullptr;

        glm::vec3 m_position = {0, 0, 0};
        glm::vec3 m_rotation = {0, 0, 0};
        glm::vec3 m_scale    = {1, 1, 1};


        Model model;
        
        std::vector<Mesh> m_meshes;
        Samplers m_samplers;

        SDL_GPUTexture* m_fallbackTex;
    };
};