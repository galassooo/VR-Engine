#include "engine.h"
// GLEW
#include <GL/glew.h>

/**
 * @brief Constructs a new Shader object.
 *
 * Initializes the internal shader ID to 0.
 */
ENG_API Eng::Shader::Shader() : id(0)
{ }

/**
 * @brief Destroys the Shader object.
 *
 * Deletes the underlying OpenGL shader if it has been created.
 */
ENG_API Eng::Shader::~Shader() {
	glDeleteShader(id);
}

/**
 * @brief Retrieves the OpenGL shader identifier.
 *
 * @return GLuint representing the shader object ID, or 0 if not created.
 */
unsigned int ENG_API Eng::Shader::getGlId() {
	return id;
}

/**
 * @brief Loads and compiles a shader from source code in memory.
 *
 * This method checks the input data, destroys any existing shader,
 * creates a new shader object via create(), uploads the source,
 * compiles it, and logs any errors up to MAX_LOGSIZE.
 *
 * @param data Pointer to a null-terminated string containing the GLSL source code.
 * @return True if compilation succeeded; false otherwise.
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

/**
 * @brief Binds and uses this shader program.
 *
 * Derived classes or external systems should ensure appropriate
 * program linking before calling this method.
 */
void ENG_API Eng::Shader::render() {
	// No default behavior; override as needed in derived classes.
}
