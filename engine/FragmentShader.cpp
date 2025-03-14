#include "Engine.h"

GLuint ENG_API Eng::FragmentShader::create()
{
	return glCreateShader(GL_FRAGMENT_SHADER);
}
