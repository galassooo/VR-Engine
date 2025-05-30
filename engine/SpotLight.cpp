#include "Engine.h"
#include <GL/freeglut.h>

/**
 * @brief Constructor for SpotLight.
 *
 * @param color The base color of the light (affects ambient, diffuse, and specular).
 * @param direction The direction of the spotlight.
 * @param cutoffAngle The cone angle of the spotlight.
 * @param fallOff The intensity falloff exponent within the spotlight cone.
 * @param radius The radius of influence for attenuation.
 */
Eng::SpotLight::SpotLight(const glm::vec3 &color, const glm::vec3 &direction, const float cutoffAngle,
                          const float fallOff, const float radius) : Light{color}, direction{direction},
                                                                     cutoffAngle{cutoffAngle}, falloff{fallOff},
                                                                     radius{radius} {
}

/**
 * @brief Configures the spotlight's parameters for rendering.
 *
 * Transforms world-space position and direction to eye-space using the
 * provided view matrix, sets shader uniforms for position, direction,
 * cutoff angle, falloff, and computes attenuation based on radius.
 *
 * @param viewMatrix Camera view matrix for converting coordinates.
 */
void Eng::SpotLight::configureLight(const glm::mat4 &viewMatrix) {
   const float radius = std::max(100.0f, this->radius);
   constexpr float constAttenuation = 1.0f;
   const float linearAttenuation = 2.0f / radius;
   const float quadraticAttenuation = 1.0f / (radius * radius);

    glm::vec4 ePos = viewMatrix * glm::vec4(getPosition(), 1.0);

    glm::vec3 eDir = glm::mat3(viewMatrix) * direction;
    eDir = glm::normalize(eDir);

    auto& sm = ShaderManager::getInstance();
    sm.setLightPosition(glm::vec3(ePos));
    sm.setLightDirection(eDir);
	sm.setLightCutoffAngle(cutoffAngle);
	sm.setLightFalloff(falloff);
	sm.setLightAttenuation(constAttenuation, linearAttenuation, quadraticAttenuation);
}


/**
 * @brief Gets the current direction of the spotlight.
 *
 * @return glm::vec3 The normalized direction of the spotlight.
 */

glm::vec3 Eng::SpotLight::getDirection() const {
   return direction;
}

/**
 * @brief Gets the current position of the spotlight.
 *
 * The position is derived from the light's final transformation matrix in the scene graph.
 *
 * @return glm::vec3 The position of the light in world coordinates.
 */

glm::vec3 Eng::SpotLight::getPosition() const {
   return glm::vec3(getFinalMatrix()[3]);
}
