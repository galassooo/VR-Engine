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
* @brief Renders the light in OpenGL.
*
* Enables the light in OpenGL and sets up its common properties.
* Derived classes should override this method to add specific functionality.
*
* @param index The index of the light (0-7, corresponding to GL_LIGHT0 to GL_LIGHT7).
*/
void Eng::Light::render() {
   //if (currentLightId < 0 || currentLightId > GL_LIGHT7)
   //   return;

   //// Dynamically compute the macro for the light
   //GLenum lightId = GL_LIGHT0 + currentLightId;

   //// Enable light
   //glEnable(GL_LIGHTING);
   //glEnable(lightId);

   //// Call the base setup
   //setupLightBase(lightId);

   auto& sm = ShaderManager::getInstance();

   sm.setLightAmbient(color * 0.2f);
   sm.setLightDiffuse(color);
   sm.setLightSpecular(color);

   configureLight(Eng::Base::getInstance().getActiveCamera()->getFinalMatrix());
}

/**
* @brief Sets the common light properties for OpenGL.
*
* This method configures the ambient, diffuse, and specular components for the light
* using the color provided during initialization.
*
* @param lightId The OpenGL light ID
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
