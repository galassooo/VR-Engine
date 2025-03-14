#include "Engine.h"
// GLEW
#include <GL/glew.h>

unsigned int ENG_API Eng::FragmentShader::create()
{
	return glCreateShader(GL_FRAGMENT_SHADER);
}
