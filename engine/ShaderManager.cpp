#include "engine.h"
// GLEW
#include <GL/glew.h>

ENG_API Eng::ShaderManager& Eng::ShaderManager::getInstance()
{
	static Eng::ShaderManager instance;
	return instance;
}

bool ENG_API Eng::ShaderManager::initialize() {
	if (initialized) {
		std::cerr << "ShaderManager::initialize() already called. Skipping reinitialization." << std::endl;
		return false;
	}

	if (!setDefaultShaders())
		return false;

	glUseProgram(program.getGlId());

	initialized = true;
	return true;
}

void ENG_API Eng::ShaderManager::setProjectionMatrix(const glm::mat4& matrix)
{
	program.setMatrix(projectionLocation, matrix);
}

void ENG_API Eng::ShaderManager::setModelViewMatrix(const glm::mat4& matrix)
{
	program.setMatrix(modelViewLocation, matrix);
}

bool ENG_API Eng::ShaderManager::setDefaultShaders() {
	const char* vs = R"(
   #version 440 core
   // Uniforms
   uniform mat4 projection;
   uniform mat4 modelview;

   // Attributes
   layout(location = 0) in vec3 in_Position;

   // Varying:
   out float dist;

   void main(void)
   {
      gl_Position = projection * modelview * vec4(in_Position, 1.0f);
      dist = abs(gl_Position.z / 100.0f);
   }
)";

	const char* fs = R"(
   #version 440 core
	
   // Varying variables from vertex shader
   in  float dist;

   out vec4 frag_Output;

   // TODO: Add material properties as Uniforms

   void main(void)
   {
      vec3 fog = vec3(1.0f, 1.0f, 1.0f);
	  vec3 color = vec3(1.0f, 0.0f, 0.0f);
      frag_Output = vec4(mix(color, fog, dist), 1.0f);
   }
)";
	// Compile vertex shader:
	std::shared_ptr<Eng::VertexShader> vertexShader = std::make_shared<Eng::VertexShader>();
	vertexShader->load(vs);

	// Compile fragment shader:
	std::shared_ptr<Eng::FragmentShader> fragmentShader = std::make_shared<Eng::FragmentShader>();
	fragmentShader->load(fs);

	// Setup shader program:
	program = Eng::Program();
	if (!program.addShader(fragmentShader).addShader(vertexShader).build())
		return false;
	program.bind(0, "in_Position");
	//program.bind(1, "in_Color");

	projectionLocation = program.getParamLocation("projection");
	modelViewLocation = program.getParamLocation("modelview");

	return true;
}
