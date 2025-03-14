#include "Engine.h"

GLuint ENG_API Eng::VertexShader::create()
{
	return glCreateShader(GL_VERTEX_SHADER);
}
