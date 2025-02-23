#include "engine.h"

/**
 * @brief Gets the view matrix of the camera.
 *
 * Computes the inverse of the camera's transformation matrix to produce
 * the view matrix, which transforms world coordinates into camera space.
 *
 * @return glm::mat4 The inverted view matrix.
 */
glm::mat4 Eng::Camera::getViewMatrix() const {
   return glm::inverse(getFinalMatrix());
}

/**
 * @brief Computes a LookAt matrix for the camera.
 *
 * This method calculates a view matrix that makes the camera point
 * towards a target position while maintaining an up direction.
 *
 * @param target The target position that the camera should look at.
 * @param customUp The custom up vector to maintain orientation.
 * @return glm::mat4 The LookAt view matrix.
 */
glm::mat4 Eng::Camera::lookAt(const glm::vec3 &target, const glm::vec3 &customUp) const {
   glm::vec3 position = glm::vec3(getFinalMatrix()[3]);
   return glm::lookAt(position, target, customUp);
}
