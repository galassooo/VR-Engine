#include "engine.h"

ENG_API Eng::Program::Program() : glId(0)
{
}

ENG_API Eng::Program::~Program()
{
	glDeleteProgram(glId);
}

ENG_API Eng::Program& Eng::Program::addShader(const std::shared_ptr<Eng::Shader>& shader)
{
	shaders.push_back(shader);
	return *this;
}

bool ENG_API Eng::Program::build()
{
	// Delete if already used:
	if (glId)
		glDeleteProgram(glId);

	// Create program:
	glId = glCreateProgram();
	if (glId == 0)
	{
		std::cout << "[ERROR] Unable to create program" << std::endl;
		return false;
	}

	for (auto& shader : shaders) {
		glAttachShader(glId, shader->getGlId());
	}

	// Link program:
	glLinkProgram(glId);

	// Verify program:
	int status;
	char buffer[MAX_LOGSIZE];
	int length = 0;
	memset(buffer, 0, MAX_LOGSIZE);

	glGetProgramiv(glId, GL_LINK_STATUS, &status);
	glGetProgramInfoLog(glId, MAX_LOGSIZE, &length, buffer);
	if (status == false)
	{
		std::cout << "[ERROR] Program link error: " << buffer << std::endl;
		return false;
	}
	glValidateProgram(glId);
	glGetProgramiv(glId, GL_VALIDATE_STATUS, &status);
	if (status == GL_FALSE)
	{
		std::cout << "[ERROR] Unable to validate program" << std::endl;
		return false;
	}

	// Done:
	return true;
}

void ENG_API Eng::Program::render()
{
	// Activate program:
	if (glId)
		glUseProgram(glId);
	else
	{
		std::cerr << "[ERROR] Invalid program for render" << std::endl;
	}
}

/**
 * Returns the param location given its variable name.
 * @param name variable name
 * @return location ID or -1 if not found
 */
int ENG_API Eng::Program::getParamLocation(const char* name)
{
	if (name == nullptr)
	{
		std::cout << "[ERROR] Invalid params" << std::endl;
		return 0;
	}

	// Return location:
	int r = glGetUniformLocation(glId, name);
	if (r == -1)
		std::cout << "[ERROR] Param '" << name << "' not found" << std::endl;
	return r;
}
