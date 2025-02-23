#pragma once

/**
 * @class DirectionalLight
 * @brief Represents a light source that emits light in a specific direction.
 *
 * The `DirectionalLight` simulates a distant light source, such as the sun,
 * where all light rays are parallel. It inherits from the `Light` base class
 * and includes additional properties and methods for managing the light's direction.
 */
class ENG_API DirectionalLight final : public Eng::Light {
public:
	DirectionalLight(const glm::vec3& color, const glm::vec3& direction);
	glm::vec3 getDirection() const;

private:
	void configureLight(const int& lightId) override;
	///< Normalized vector indicating the global direction of light rays
	glm::vec3 direction;
};
