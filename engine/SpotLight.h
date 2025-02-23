#pragma once

/**
 * @class SpotLight
 * @brief Represents a light source that emits light in a specific direction with a cone-shaped area of effect.
 *
 * The `SpotLight` simulates a focused light source, such as a flashlight or stage light. It is characterized by its
 * direction, cutoff angle, and falloff factor, which define the shape and intensity distribution of the light beam.
 * It inherits common properties from the `Light` base class.
 */
class ENG_API SpotLight : public Eng::Light {
public:
	SpotLight(const glm::vec3& color, const glm::vec3& direction, float cutoffAngle, float fallOff, float radius);
	glm::vec3 getDirection() const;
	glm::vec3 getPosition() const;

private:
	void configureLight(const int& lightId) override;
	///> Direction vector of the spotlight's beam
	glm::vec3 direction;
	///> The angle in degrees that defines the spotlight's cone
	float cutoffAngle;
	///> Intensity falloff factor within the spotlight's cone
	float falloff;
	///> Maximum distance the light can reach before full attenuation
	float radius;
};
