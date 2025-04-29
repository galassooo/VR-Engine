#pragma once

/**
 * @class Material
 * @brief Represents the material properties of a 3D object, including color, shininess, and optional textures.
 *
 * The `Material` class encapsulates visual properties used in rendering, such as color, shininess for specular highlights,
 * and an optional diffuse texture.
 */
class ENG_API Material : Eng::Object {
public:
   Material(const glm::vec3 &albedo, float alpha, float shininess, const glm::vec3 emission);

   void render() override;

   void setDiffuseTexture(const std::shared_ptr<Eng::Texture> &texture);
   std::shared_ptr<Eng::Texture> getDiffuseTexture() const;

   glm::vec3 getAlbedo() const;
   float getAlpha() const;
   glm::vec4 getAlbedoWithAlpha() const;

private:
   ///> Albedo with RGBA (alpha included)
   glm::vec4 albedo;
   glm::vec3 emission;
   ///> shininess component
   float shininess;
   ///> pointer to texture (nullptr if not present)
   std::shared_ptr<Eng::Texture> diffuseTexture;
};
