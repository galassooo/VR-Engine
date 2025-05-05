#include "engine.h"

#include <typeinfo>

#include <GL/glew.h>

Eng::ColorPass::ColorPass(std::shared_ptr<Eng::Program> program, BlendingMode mode, glm::vec3 globalLightColor)
	: RenderPass(program), mode(mode), globalLightColor(globalLightColor) {
}

void Eng::ColorPass::setGlobalLightColor(glm::vec3& color) {
	globalLightColor = color;
}

void Eng::ColorPass::configRender() {
	if (mode == Additive) {
		// Set additive blending mode
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);
		//glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ZERO, GL_ONE); // ignores alpha channel in blendind

		// Disable depth writing
		glDepthMask(GL_FALSE);
		// Set depth test function to less or equal to allow overlapping
		glDepthFunc(GL_LEQUAL);
	}
	else if (mode == Transparent) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glDepthMask(GL_FALSE);          // non scriviamo z
		glDepthFunc(GL_LEQUAL);
	}
	else {
		// Set default blending mode
		glDisable(GL_BLEND);
		// Enable depth writing
		glDepthMask(GL_TRUE);
		// Set default depth test function
		glDepthFunc(GL_LESS);
	}

	if (!shaderManager.loadProgram(renderProgram)) {
		std::cerr << "ERROR: Failed to load " << typeid(renderProgram).name() << " program" << std::endl;
	}

	/// Set specific shader pass-level variables

	// Load projection matrix
	shaderManager.setProjectionMatrix(eyeProjectionMatrix);

	// Send eye front vector: this is the camera front vector in world coordinates
	// which corresponds to the third column of the view matrix
	glm::vec3 eyeFront = -glm::vec3(glm::transpose(glm::mat3(eyeViewMatrix))[2]);
	shaderManager.setEyeFront(eyeFront);

	shaderManager.setGlobalLightColor(globalLightColor);
}

void Eng::ColorPass::perElementConfig(const std::shared_ptr<Eng::ListElement>& element) {
	/// Set only element specific shader variables

	glm::mat4 modelMatrix = element->getWorldCoordinates();

	// Generate modelView matrix
	glm::mat4 modelViewMatrix = eyeViewMatrix * modelMatrix;

	// Send 4x4 modelview matrix
	shaderManager.setModelViewMatrix(modelViewMatrix);

	// Send lightSpaceModel matrix
	glm::mat4 modelLightMatrix = lightSpaceMatrix * modelMatrix;
	shaderManager.setLightSpaceMatrix(modelLightMatrix);

	// Send 3x3 inverse-transpose for normals
	glm::mat3 normalMat = glm::inverseTranspose(glm::mat3(modelViewMatrix));
	shaderManager.setNormalMatrix(normalMat);
}