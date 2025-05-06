#include "engine.h"
#include <GL/freeglut.h>

/**
 * @brief Constructor for PointLight.
 *
 * @param color The base color of the light (affects ambient, diffuse, and specular).
 * @param attenuation The attenuation factor, controlling how light intensity decreases with distance.
 */
Eng::PointLight::PointLight(const glm::vec3 &color, const float attenuation) : Light{color},
                                                                               attenuation{attenuation} {
}

/**
 * @brief Configures the point light's position and attenuation in the shader.
 *
 * Transforms the light's world-space position by the view matrix to eye-space,
 * updates the ShaderManager with the position, and computes attenuation terms
 * based on the configured radius derived from the attenuation factor.
 *
 * @param viewMatrix Camera view matrix for converting world to eye-space coordinates.
 */
void Eng::PointLight::configureLight(const glm::mat4 &viewMatrix) {
    glm::vec4 wPos(getPosition(), 1.0f);

    glm::vec4 ePos = viewMatrix * wPos;

    auto& sm = ShaderManager::getInstance();
    sm.setLightPosition(glm::vec3(ePos));

    // Set attenuation
   float radius = std::max(10.0f, attenuation);
   float constAttenuation = 1.0f;
   float linearAttenuation = 2.0f / radius;
   float quadraticAttenuation = 1.0f / (radius * radius);
   sm.setLightAttenuation(constAttenuation, linearAttenuation, quadraticAttenuation);
}

/**
 * @brief Gets the current position of the point light.
 *
 * The position is derived from the light's final transformation matrix in the scene graph.
 *
 * @return glm::vec3 The position of the light in world coordinates.
 */
glm::vec3 Eng::PointLight::getPosition() const {
   return glm::vec3(getFinalMatrix()[3]);
}
