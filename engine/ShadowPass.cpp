#include "engine.h"

#include <GL/glew.h>

Eng::ShadowPass::ShadowPass(std::shared_ptr<Eng::Program>& program, std::shared_ptr<Eng::Fbo>& shadowMapFbo)
: RenderPass(program), shadowMapFbo(shadowMapFbo){}

void Eng::ShadowPass::perElementConfig(const std::shared_ptr<Eng::ListElement>& element) {
	// Send lightSpaceModel matrix
	glm::mat4 modelMatrix = element->getWorldCoordinates();
	glm::mat4 modelLightMatrix = lightSpaceMatrix * modelMatrix;
	shaderManager.setLightSpaceMatrix(modelLightMatrix);
}

void Eng::ShadowPass::configRender() {
	shaderManager.loadProgram(renderProgram);
	
	// Activate and clean the shadow map FBO
	shadowMapFbo->render();

	// Enable depth writing
	glDepthMask(GL_TRUE);

	// Clear depth buffer
	glClear(GL_DEPTH_BUFFER_BIT);
}