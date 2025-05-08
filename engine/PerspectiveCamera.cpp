#include "Engine.h"

/**
 * @brief Constructor for PerspectiveCamera.
 *
 * Initializes a perspective projection camera with the specified field of view,
 * aspect ratio, and clipping planes.
 *
 * @param fov The field of view (in degrees) in the vertical direction.
 * @param aspect The aspect ratio of the view (width divided by height).
 * @param nearClip The near depth clipping plane.
 * @param farClip The far depth clipping plane.
 */
Eng::PerspectiveCamera::PerspectiveCamera(float fov, float aspect, float nearClip, float farClip) : fov{fov},
   aspect{aspect}, nearClip{nearClip}, farClip{farClip} {
}

/**
 * @brief Computes the perspective projection matrix.
 *
 * This method calculates a projection matrix for rendering a scene using
 * perspective projection, where objects farther from the camera appear smaller.
 *
 * @return glm::mat4 The perspective projection matrix.
 */
glm::mat4 Eng::PerspectiveCamera::getProjectionMatrix() const {
   return glm::perspective(glm::radians(fov), aspect, nearClip, farClip);
}

/**
 * @brief Updates the aspect ratio of the camera.
 *
 * This method allows you to dynamically change the aspect ratio of the camera,
 * typically when the viewport dimensions are resized.
 *
 * @param newAspect The new aspect ratio (width divided by height).
 */
void Eng::PerspectiveCamera::setAspect(float newAspect) {
   aspect = newAspect;
}

float Eng::PerspectiveCamera::getNearClip() const{ return nearClip; }
float Eng::PerspectiveCamera::getFarClip() const { return farClip; }
