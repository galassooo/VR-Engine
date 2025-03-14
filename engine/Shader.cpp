#include "engine.h"

ENG_API Eng::Shader::Shader() : glId(0)
{ }
ENG_API Eng::Shader::~Shader() {
	glDeleteShader(glId);
}

GLuint ENG_API Eng::Shader::getGlId() {
	return glId;
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
	if (glId)
		glDeleteShader(glId);

	// Load program:
	glId = create();
	if (glId == 0)
	{
		std::cout << "[ERROR] Unable to create shader object" << std::endl;
		return false;
	}
	glShaderSource(glId, 1, (const char**)&data, NULL);
	glCompileShader(glId);

	// Verify shader:
	int status;
	char buffer[MAX_LOGSIZE];
	int length = 0;
	memset(buffer, 0, MAX_LOGSIZE);

	glGetShaderiv(glId, GL_COMPILE_STATUS, &status);
	glGetShaderInfoLog(glId, MAX_LOGSIZE, &length, buffer);
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
