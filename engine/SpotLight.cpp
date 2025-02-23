#include "engine.h"
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
 * @brief Renders the spotlight in OpenGL.
 *
 * Configures the spotlight's position, direction, cutoff angle, and attenuation.
 *
 * @param index The index of the light (0-7, corresponding to GL_LIGHT0 to GL_LIGHT7).
 */
void Eng::SpotLight::configureLight(const int &lightId) {
   // Get position from transformation matrix
   const GLfloat lightPosition[] = {0, 0, 0, 1.0f};

   // Set position and direction
   glLightfv(lightId, GL_POSITION, lightPosition);
   glLightfv(lightId, GL_SPOT_DIRECTION, glm::value_ptr(direction));

   // Set spot parameters
   glLightf(lightId, GL_SPOT_CUTOFF, cutoffAngle);
   glLightf(lightId, GL_SPOT_EXPONENT, falloff);

   const float radius = std::max(100.0f, this->radius);
   constexpr float constAttenuation = 1.0f;
   const float linearAttenuation = 2.0f / radius;
   const float quadraticAttenuation = 1.0f / (radius * radius);

   glLightf(lightId, GL_CONSTANT_ATTENUATION, constAttenuation);
   glLightf(lightId, GL_LINEAR_ATTENUATION, linearAttenuation);
   glLightf(lightId, GL_QUADRATIC_ATTENUATION, quadraticAttenuation);
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
