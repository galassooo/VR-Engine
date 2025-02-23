#include "engine.h"

/**
 * @brief Constructor for OrthographicCamera.
 *
 * Initializes an orthographic projection camera with the specified clipping planes.
 *
 * @param left The left vertical clipping plane.
 * @param right The right vertical clipping plane.
 * @param bottom The bottom horizontal clipping plane.
 * @param top The top horizontal clipping plane.
 * @param nearClip The near depth clipping plane.
 * @param farClip The far depth clipping plane.
 */
Eng::OrthographicCamera::OrthographicCamera(const float left, const float right, const float bottom, const float top, const float nearClip,
                                            const float farClip) : left{left}, right{right}, bottom{bottom}, top{top},
                                                                   nearClip{nearClip}, farClip{farClip} {
}

/**
 * @brief Computes the orthographic projection matrix.
 *
 * This method calculates a projection matrix for rendering a scene using
 * orthographic projection.
 *
 * @return glm::mat4 The orthographic projection matrix.
 */
glm::mat4 Eng::OrthographicCamera::getProjectionMatrix() const {
   return glm::ortho(left, right, bottom, top, nearClip, farClip);
}
