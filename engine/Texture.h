#pragma once

/**
 * @class Texture
 * @brief Represents a texture used in rendering, loaded from an image file and applied to a 3D object.
 *
 * The `Texture` class provides functionality for loading a texture from a file, binding it to a specific texture
 * unit, and managing its OpenGL texture ID.
 */
class ENG_API Texture : public Eng::Object {
public:
   explicit Texture(const std::string &filePath = "");
   ~Texture();

   bool loadFromFile(const std::string &filePath);
   void render() override;

   int getWidth() const { return width; }
   int getHeight() const { return height; }

private:
   ///> OpenGL texture ID.
   unsigned int textureID;
   ///> File path of the texture.
   std::string filePath;
   ///> Texture width.
   int width;
   ///> Texture height.
   int height;

   void configureTextureParameters();
};
