#pragma once

/**
 * @class PointLight
 * @brief Represents a light source that emits light in all directions from a single point.
 *
 * The `PointLight` simulates a localized light source, such as a light bulb,
 * with intensity diminishing over distance based on an attenuation factor.
 * It inherits common light properties from the Light base class.
 */
class ENG_API PointLight : public Eng::Light {
public:
   PointLight(const glm::vec3 &color, float attenuation);

   glm::vec3 getPosition() const;

private:
   void configureLight(const glm::mat4 &viewMatrix) override;
   ///> radius is written as attenuation
   float attenuation;
};
