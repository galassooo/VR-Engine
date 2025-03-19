#pragma once

/**
 * @class Light
 * @brief Represents a base class for lights in the scene graph.
 *
 * The `Light` class defines the common properties of lights, such as color and intensity,
 * and integrates with the scene graph by inheriting from `Node`.
 */
class ENG_API Light : public Eng::Node {
public:
   explicit Light(const glm::vec3 &color);

   void setColor(const glm::vec3 &color);
   void render() override;

private:
   virtual void setupLightBase(const int &lightId) const;

   int currentLightId;

protected:
   static int lightID;

   Light();
   ~Light();
   virtual void configureLight(const glm::mat4 &viewMatrix) = 0;

   ///> RGB color of the light source, affects ambient, diffuse, and specular components
   glm::vec3 color;
};
