#include "engine.h"
#include <GL/freeglut.h>

int Eng::Light::lightID = 0;

/**
* @brief Constructor for Light.
*
* @param color The base color of the light (affects ambient, diffuse, and specular).
*/
Eng::Light::Light(const glm::vec3& color) : color{ color }, currentLightId{ lightID } {
    lightID++;
}

/**
* @brief Destructor for Light.
*
* Decrements static int for light IDs.
*/
Eng::Light::~Light() {
    --lightID;
}

/**
 * @brief Renders the light by uploading its ambient, diffuse, and specular
 * properties to the shader and configuring any subclass-specific settings.
 *
 * Uses the ShaderManager to set common material-like properties, then
 * calls configureLight() with the inverse of the head node's local matrix
 * to place the light in world space.
 */
void Eng::Light::render() {
   auto& sm = ShaderManager::getInstance();

   sm.setLightAmbient(color * 0.2f);
   sm.setLightDiffuse(color * 1.5f);
   sm.setLightSpecular(color * 1.5f);

   // Because the head position shall not depend on its parent, we need to
   // use the inverse of its local matrix
   configureLight(glm::inverse(Eng::Base::getInstance().getHeadNode()->getLocalMatrix()));
}

/**
 * @brief Configures the OpenGL light parameters for this light.
 *
 * Sets the GL_AMBIENT, GL_DIFFUSE, and GL_SPECULAR components on the
 * specified OpenGL light ID using the stored color.
 *
 * @param lightId OpenGL light identifier.
 */
void Eng::Light::setupLightBase(const int &lightId) const {
   // Set light properties
   GLfloat lightColor[] = {color.r, color.g, color.b, 1.0f};

   // Set the light properties
   glLightfv(lightId, GL_DIFFUSE, lightColor);
   glLightfv(lightId, GL_AMBIENT, lightColor);
   glLightfv(lightId, GL_SPECULAR, lightColor);
}

/**
 * @brief Sets the color of the light source
 *
 * @param color New RGB color values for the light
 */

void Eng::Light::setColor(const glm::vec3 &color) {
   this->color = color;
}
