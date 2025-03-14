#include "engine.h"
#include <GL/freeglut.h>

/**
 * @brief Constructor for Material class with RGBA albedo and shininess.
 *
 * @param albedo The diffuse color of the material (RGB).
 * @param alpha Transparency value (0.0 = fully transparent, 1.0 = fully opaque).
 * @param shininess The shininess factor for specular highlights.
 */
Eng::Material::Material(const glm::vec3 &albedo, const float alpha, const float shininess)
   : albedo(glm::vec4(albedo, alpha)), shininess(shininess) {
}

/**
 * @brief Applies the material properties to OpenGL.
 *
 * Configures the material's ambient, diffuse, and specular properties
 * and sets the shininess for rendering.
 *
 * @param index The index of the material, typically used to identify material order.
 */
void Eng::Material::render() {
   if(getAlpha() < 1.0f) {
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   }
   const GLfloat ambient[] = {albedo.r * 0.2f, albedo.g * 0.2f, albedo.b * 0.2f, 1.0f};
   const GLfloat diffuse[] = {albedo.r * 0.6f, albedo.g * 0.6f, albedo.b * 0.6f, albedo.a};
   const GLfloat specular[] = {albedo.r * 0.4f, albedo.g * 0.4f, albedo.b * 0.4f, 1.0f};
   const GLfloat shininess[] = {(1.0f - std::sqrt(this->shininess)) * 128.0f};
   
   /*   Unsupported 4.4
   glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
   glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
   glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
   glMaterialfv(GL_FRONT, GL_SHININESS, shininess);
   

   if (diffuseTexture) {
      glEnable(GL_TEXTURE_2D);
      diffuseTexture->render();
   } else {
      glDisable(GL_TEXTURE_2D);
   }
   */
   if(getAlpha() >= 1.0f) {
      glDisable(GL_BLEND);
   }
}

/**
 * @brief Sets the diffuse texture for the material.
 *
 * Associates a texture with the material to define its surface appearance.
 *
 * @param texture A shared pointer to the texture to be applied.
 */

void Eng::Material::setDiffuseTexture(const std::shared_ptr<Eng::Texture> &texture) {
   diffuseTexture = texture;
}

/**
 * @brief Retrieves the diffuse texture of the material.
 *
 * Returns the texture associated with the material, if any.
 *
 * @return std::shared_ptr<Eng::Texture> A shared pointer to the diffuse texture, or nullptr if no texture is set.
 */
std::shared_ptr<Eng::Texture> Eng::Material::getDiffuseTexture() const {
   return diffuseTexture;
}

/**
 * @brief Retrieves the albedo (RGB only) of the material.
 *
 * @return glm::vec3 The albedo color (RGB).
 */

glm::vec3 Eng::Material::getAlbedo() const {
   return glm::vec3(albedo.r, albedo.g, albedo.b);
}

/**
 * @brief Retrieves the alpha (transparency) of the material.
 *
 * @return float The alpha value (0.0 = fully transparent, 1.0 = fully opaque).
 */
float Eng::Material::getAlpha() const {
   return albedo.a;
}

/**
 * @brief Retrieves the albedo with alpha (RGBA) of the material.
 *
 * @return glm::vec4 The albedo color with transparency.
 */
glm::vec4 Eng::Material::getAlbedoWithAlpha() const {
   return albedo;
}
