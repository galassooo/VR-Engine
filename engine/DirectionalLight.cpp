#include "engine.h"
#include <GL/freeglut.h>

/**
 * @brief Constructor for DirectionalLight.
 *
 * @param color The base color of the light (affects ambient, diffuse, and specular).
 * @param direction The direction of the light rays.
 */

Eng::DirectionalLight::DirectionalLight(const glm::vec3 &color, const glm::vec3 &direction) : Light{color},
   direction{glm::normalize(direction)} {
}

/**
 * @brief Configures the directional light for rendering.
 *
 * Transforms the stored direction vector by the view matrix and uploads
 * the resulting eye-space direction to the ShaderManager.
 *
 * @param viewMatrix The camera view matrix used to transform world-space direction.
 */
void Eng::DirectionalLight::configureLight(const glm::mat4 &viewMatrix) {
   glm::vec3 wDir = glm::normalize(glm::mat3(localMatrix) * direction);
   glm::vec3 eDir = glm::mat3(viewMatrix) * wDir;
   eDir = glm::normalize(eDir);

   auto& sm = ShaderManager::getInstance();
   sm.setLightDirection(eDir);
}

/**
 * @brief Retrieves the normalized direction of the light.
 *
 * @return glm::vec3 Normalized world-space direction vector.
 */
glm::vec3 Eng::DirectionalLight::getDirection() const {
   return direction;
}


/**
 * @brief Computes a view matrix for the light's perspective.
 *
 * Calculates a lookAt matrix using the center of the provided frustum
 * corners and the light's direction at a distance of maxRange.
 *
 * @param frustumCorners List of eight world-space corner positions of the camera frustum.
 * @param maxRange Maximum distance behind the center along the light direction to position the light.
 * @return glm::mat4 Light-space view matrix, or identity if inputs are invalid.
 */
glm::mat4 Eng::DirectionalLight::getLightViewMatrix(const std::vector<glm::vec3>& frustumCorners, float maxRange) {
    if (frustumCorners.empty()) {
        std::cerr << "ERROR: Frustum corners array is empty!" << std::endl;
        return glm::mat4(1.0f);
    }

    // calculate the center by finding the median point within the frustum
    glm::vec3 center(0.0f);
    for (const auto& corner : frustumCorners) {
        center += corner;
    }
    center /= static_cast<float>(frustumCorners.size());

    glm::vec3 wDir = glm::normalize(glm::mat3(localMatrix) * direction);
    if (glm::length(wDir) == 0.0f) {
        std::cerr << "ERROR: Direction vector is zero!" << std::endl;
    }

    glm::vec3 lightPos = center - wDir * maxRange;

    glm::vec3 up = glm::abs(glm::dot(wDir, glm::vec3(0, 1, 0))) > 0.99f ?
        glm::vec3(0, 0, 1) : glm::vec3(0, 1, 0);

    glm::mat4 lightView = glm::lookAt(lightPos, center, up);

    return lightView;
}
