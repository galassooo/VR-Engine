#include "Engine.h"
// GLEW
#include <GL/glew.h>

unsigned int ENG_API Eng::VertexShader::create()
{
	return glCreateShader(GL_VERTEX_SHADER);
}
