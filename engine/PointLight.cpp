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
 * @brief Renders the point light in OpenGL.
 *
 * Configures the light's position and attenuation settings.
 *
 * @param lightId The index of the light
 */
void Eng::PointLight::configureLight(const glm::mat4 &viewMatrix) {
   // Set light position, the value is hardcoded to avoid double position when using GL_POSITION 
   /*GLfloat lightPosition[] = {0, 0, 0, 1.0f};*/

   // Not supported on OpenGl 4.4
   /*glLightfv(lightId, GL_POSITION, lightPosition);

   glLightf(lightId, GL_SPOT_CUTOFF, 180.0f);
   glLightf(lightId, GL_SPOT_EXPONENT, 0.0f);*/

   /*glLightf(lightId, GL_CONSTANT_ATTENUATION, constAttenuation);
   glLightf(lightId, GL_LINEAR_ATTENUATION, linearAttenuation);
   glLightf(lightId, GL_QUADRATIC_ATTENUATION, quadraticAttenuation);*/
    
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
