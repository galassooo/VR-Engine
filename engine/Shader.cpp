#include "engine.h"
// GLEW
#include <GL/glew.h>

ENG_API Eng::Shader::Shader() : id(0)
{ }
ENG_API Eng::Shader::~Shader() {
	glDeleteShader(id);
}

unsigned int ENG_API Eng::Shader::getGlId() {
	return id;
}

/**
 * Loads and compiles a shader from source code stored in memory.
 * @param data pointer to the string containing the source code
 * @return true/false on success/failure
 */
bool ENG_API Eng::Shader::load(const char* data) {
	if (data == nullptr)
	{
		std::cout << "[ERROR] Invalid params" << std::endl;
		return false;
	}

	// Destroy if already loaded:
	if (id)
		glDeleteShader(id);

	// Load program:
	id = create();
	if (id == 0)
	{
		std::cout << "[ERROR] Unable to create shader object" << std::endl;
		return false;
	}
	glShaderSource(id, 1, (const char**)&data, NULL);
	glCompileShader(id);

	// Verify shader:
	int status;
	char buffer[MAX_LOGSIZE];
	int length = 0;
	memset(buffer, 0, MAX_LOGSIZE);

	glGetShaderiv(id, GL_COMPILE_STATUS, &status);
	glGetShaderInfoLog(id, MAX_LOGSIZE, &length, buffer);
	if (status == false)
	{
		std::cout << "[ERROR] Shader not compiled: " << buffer << std::endl;
		return false;
	}

	// Done:
	return true;
}

void ENG_API Eng::Shader::render() {

}
