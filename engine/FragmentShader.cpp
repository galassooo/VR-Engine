#include "Engine.h"
// GLEW
#include <GL/glew.h>

/**
 * @brief Creates the OpenGL fragment shader object.
 *
 * Uses glCreateShader with GL_FRAGMENT_SHADER to instantiate
 * a fragment-stage shader. This is called by the base Shader
 * load routine prior to source upload and compilation.
 *
 * @return GLuint ID of the created shader object, or 0 on failure.
 */
unsigned int ENG_API Eng::FragmentShader::create()
{
	return glCreateShader(GL_FRAGMENT_SHADER);
}
