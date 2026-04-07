#pragma once

#include "ecs/System.h"
#include "ecs/World.h"
#include "renderer/Shader.h"
#include "renderer/Camera.h"
#include "renderer/MeshCache.h"
#include "components/TransformComponent.h"
#include "components/MeshComponent.h"
#include "components/MaterialComponent.h"

class RenderSystem : public System {
public:
    RenderSystem(Shader& neonShader, Shader& skyboxShader,
                 Camera& camera, MeshCache& meshes)
        : m_neonShader(neonShader), m_skyboxShader(skyboxShader),
          m_camera(camera), m_meshes(meshes) {}

    void setTime(float t) { m_time = t; }

    void update(World& world, float /*dt*/) override {
        glm::mat4 viewProj = m_camera.projectionMatrix() * m_camera.viewMatrix();
        glm::vec3 lightDir = glm::normalize(glm::vec3(0.5f, 1.0f, 0.3f));
        glm::vec3 viewPos = m_camera.position();

        // Render all neon entities
        m_neonShader.bind();
        m_neonShader.setMat4("u_viewProj", viewProj);
        m_neonShader.setVec3("u_lightDir", lightDir);
        m_neonShader.setVec3("u_viewPos", viewPos);
        m_neonShader.setFloat("u_time", m_time);

        world.each<TransformComponent, MeshComponent, MaterialComponent>(
            [&](Entity, TransformComponent& t, MeshComponent& mc, MaterialComponent& mat) {
                m_neonShader.setMat4("u_model", t.matrix());
                m_neonShader.setVec3("u_baseColor", mat.baseColor);
                m_neonShader.setVec3("u_emissionColor", mat.emissionColor);
                m_neonShader.setFloat("u_emissionStrength", mat.emissionStrength);
                m_meshes.get(mc.type).draw();
            }
        );
    }

private:
    Shader& m_neonShader;
    Shader& m_skyboxShader;
    Camera& m_camera;
    MeshCache& m_meshes;
    float m_time = 0.0f;
};
