#include "engine.h"

// GLEW
#include <GL/glew.h>

#include <GL/freeglut.h>

/**
 * @brief Constructor for Material class with RGBA albedo and shininess.
 *
 * @param albedo The diffuse color of the material (RGB).
 * @param alpha Transparency value (0.0 = fully transparent, 1.0 = fully opaque).
 * @param shininess The shininess factor for specular highlights.
 */
Eng::Material::Material(const glm::vec3 &albedo, const float alpha, const float shininess, const glm::vec3 emission)
   : albedo(glm::vec4(albedo, alpha)), shininess(shininess), emission(emission) {
}

/**
 * @brief Applies this material's properties to the current shader.
 *
 * Uploads ambient, diffuse, specular, shininess, and emission
 * parameters to the ShaderManager, binds the diffuse texture if any,
 * and configures OpenGL blending based on alpha transparency.
 *
 * @param index Optional material index (unused in this implementation).
 */
void Eng::Material::render() {
    // Remember previous OpenGL blending state
   bool blendingEnabled = glIsEnabled(GL_BLEND);
   //std::cout << "(Material) Blending: " << (blendingEnabled ? "Enabled" : "Disabled") << std::endl;
   GLint srcRGB = 0;
   GLint dstRGB = 0;

   if(getAlpha() < 1.0f) {
       if (blendingEnabled) {
           glGetIntegerv(GL_BLEND_SRC_RGB, &srcRGB);
           glGetIntegerv(GL_BLEND_DST_RGB, &dstRGB);
       }
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   }

   const GLfloat ambient[] = {albedo.r * 0.2f, albedo.g * 0.2f, albedo.b * 0.2f, 1.0f};
   const GLfloat diffuse[] = {albedo.r * 0.6f, albedo.g * 0.6f, albedo.b * 0.6f, albedo.a};
   const GLfloat specular[] = {albedo.r * 0.4f, albedo.g * 0.4f, albedo.b * 0.4f, 1.0f};
   const GLfloat shininess[] = {(1.0f - std::sqrt(this->shininess)) * 128.0f};

   ShaderManager& sm = ShaderManager::getInstance();

   glm::vec3 amb(ambient[0], ambient[1], ambient[2]);
   glm::vec3 dif(diffuse[0], diffuse[1], diffuse[2]);
   glm::vec3 spe(specular[0], specular[1], specular[2]);

   float shin = shininess[0];

   sm.setMaterialAmbient(amb);
   sm.setMaterialDiffuse(dif);
   sm.setMaterialSpecular(spe);
   sm.setMaterialShininess(shin);
   sm.setMaterialEmission(emission * 2.f);
   sm.setMaterialAmbient(glm::vec3(albedo.r * 0.2f, albedo.g * 0.2f, albedo.b * 0.2f));
   sm.setMaterialDiffuse(glm::vec3(albedo.r * 0.6f, albedo.g * 0.6f, albedo.b * 0.6f));
   sm.setMaterialSpecular(glm::vec3(albedo.r * 0.4f, albedo.g * 0.4f, albedo.b * 0.4f));
   sm.setMaterialShininess((1.0f - std::sqrt(this->shininess)) * 128.0f);

   //Texture
   if (diffuseTexture) {
       sm.setUseTexture(true);
       diffuseTexture->render();
   }
   else {
       sm.setUseTexture(false);
   }

   // Reset previous OpenGL blending state
   if (getAlpha() < 1.0f) {
       if (blendingEnabled) {
           glEnable(GL_BLEND);
           glBlendFunc(srcRGB, dstRGB);
       } else {
           glDisable(GL_BLEND);
       }
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
 * @brief Sets the alpha channel for the material.
 *
 * @param new alpha to be set
 */
void Eng::Material::setAlpha(const float newAlpha) {
    albedo = glm::vec4(albedo.r, albedo.g, albedo.b, newAlpha);
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
 * @brief Retrieves the emission  of the material.
 *
 * @return vec3 containing emission values
 */
glm::vec3 Eng::Material::getEmission() const {
    return emission;
}

/**
 * @brief Sets the emission for the material.
 *
 * @param a vec3 containing the new emissive values
 */
void Eng::Material::setEmission(glm::vec3 emissionNew) {
    emission = emissionNew;
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
