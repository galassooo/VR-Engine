#pragma once

/**
 * @class Camera
 * @brief Represents a camera in the scene graph, responsible for providing view and projection matrices.
 */
class ENG_API Camera : public Eng::Node {
public:
	virtual glm::mat4 getProjectionMatrix() const = 0;
	glm::mat4 getViewMatrix() const;
	glm::mat4 lookAt(const glm::vec3& target, const glm::vec3& customUp = glm::vec3(0, 1, 0)) const;
};
