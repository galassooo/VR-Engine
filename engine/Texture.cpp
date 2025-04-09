#include "engine.h"
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <FreeImage.h>

/**
 * @brief Constructs a Texture object and optionally loads a texture from a file.
 * @param filePath Path to the texture file to load (optional).
 */
Eng::Texture::Texture(const std::string &filePath)
   : textureID(0), width(0), height(0), filePath(filePath) {
   if (!filePath.empty()) {
      loadFromFile(filePath);
   }
}

/**
 * @brief Destructor for the Texture class. Cleans up OpenGL texture resources.
 */
Eng::Texture::~Texture() {
   if (textureID) {
      glDeleteTextures(1, &textureID);
   }
}

/**
 * @brief Loads a texture from a file and uploads it to OpenGL.
 * @param filePath Path to the texture file.
 * @return True if the texture was loaded successfully, false otherwise.
 */
bool Eng::Texture::loadFromFile(const std::string &filePath) {
   FIBITMAP *bitmap = FreeImage_Load(FreeImage_GetFileType(filePath.c_str(), 0), filePath.c_str());
   if (!bitmap) {
      std::cerr << "Failed to load texture: " << filePath << std::endl;
      return false;
   }

   // Converti sempre l'immagine in RGBA 32-bit
   FIBITMAP *bitmap32 = FreeImage_ConvertTo32Bits(bitmap);
   FreeImage_Unload(bitmap);
   if (!bitmap32) {
      std::cerr << "Failed to convert texture to 32 bits: " << filePath << std::endl;
      return false;
   }


   // Usa sempre RGBA come formato
   if (textureID) {
      glDeleteTextures(1, &textureID);
   }

   width = FreeImage_GetWidth(bitmap32);
   height = FreeImage_GetHeight(bitmap32);

   glGenTextures(1, &textureID);
   glBindTexture(GL_TEXTURE_2D, textureID);

   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                width,
                height,
                0, GL_BGRA_EXT, GL_UNSIGNED_BYTE,
                FreeImage_GetBits(bitmap32));

   // Genera mipmaps
   gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA8,
                     width,
                     height,
                     GL_BGRA_EXT, GL_UNSIGNED_BYTE,
                     FreeImage_GetBits(bitmap32));


   configureTextureParameters();

   FreeImage_Unload(bitmap32);
   return true;
}

/**
 * @brief Configures default OpenGL texture parameters.
 */
void Eng::Texture::configureTextureParameters() {
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

/**
 * @brief Renders the texture, applying it to the OpenGL context.
 * @param index Optional index parameter (default is -1).
 */
void Eng::Texture::render() {
   // Activate the correct texture unit based on the shader manager parameters
   glActiveTexture(GL_TEXTURE0 + ShaderManager::DIFFUSE_TEXURE_UNIT);
   // Bind the texture to the current OpenGL context in the given unit.
   glBindTexture(GL_TEXTURE_2D, textureID);
}
