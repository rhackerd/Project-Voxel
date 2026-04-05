#pragma once

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/trigonometric.hpp>

namespace N::Graphics {
    struct CameraUBO {
        glm::mat4 view;
        glm::mat4 proj;
        glm::mat4 viewProj;
    };

    class Camera {
        public:
            Camera() : pos(0,0,0), target(0,0,1), fov(45.0f), near(0.1f), far(1000.0f) {}

            Camera(glm::vec3 pos, glm::vec3 target, float fov, float near, float far)
                : pos(pos), target(target), fov(fov), near(near), far(far) {}

            ~Camera() {}

        public:
            void update(float aspectRatio) {
                ubo.view = glm::lookAt(pos, target, glm::vec3(0,1,0));
                ubo.proj = glm::perspectiveRH_ZO(glm::radians(fov), aspectRatio, near, far);
                ubo.proj[1][1] *= -1; // For vulkan
                ubo.viewProj = ubo.proj * ubo.view;
            };

            CameraUBO getUBO() { return ubo; }

            void setPos(glm::vec3 pos) { this->pos = pos; dirty = true; }
            void setTarget(glm::vec3 target) { this->target = target; dirty = true; }
            void setFOV(float fov) { this->fov = fov; dirty = true; }
            void setNear(float near) { this->near = near; dirty = true; }
            void setFar(float far) { this->far = far; dirty = true; }

            void move(float x, float y, float z) { pos += glm::vec3(x,y,z); dirty = true; }
            
            // Now getters
            glm::vec3 getPos() { return pos; }
            glm::vec3 getTarget() { return target; }
            float getFOV() { return fov; }
            float getNear() { return near; }
            float getFar() { return far; }

        private:
            glm::vec3 pos;
            glm::vec3 target;
            float fov, near, far;
            CameraUBO ubo;

            bool dirty = false;
    };
};