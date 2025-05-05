#include "engine.h"

#include <GL/glew.h>

Eng::RenderPass::RenderPass(std::shared_ptr<Eng::Program>& program) : renderProgram{program} {

}

bool Eng::RenderPass::init() {
	initialized = true;
	return initialized;
}

void Eng::RenderPass::start(const glm::mat4& eyeProjectionMatrix, const glm::mat4& eyeViewMatrix, const glm::mat4& lightSpaceMatrix) {
	if (!initialized) {
		if (!init()) {
			std::cerr << "ERROR: Render pass failed to initialize." << std::endl;
			return;
		}
	}

	if (started) {
		std::cerr << "ERROR: Render pass already started. Call stop() before start()." << std::endl;
		return;
	}

	// rembember current OpenGL blendig state
	previousState.blendingEnabled = glIsEnabled(GL_BLEND);
	if (previousState.blendingEnabled) {
		glGetIntegerv(GL_BLEND_SRC_RGB, &previousState.srcRGB);
		glGetIntegerv(GL_BLEND_DST_RGB, &previousState.dstRGB);
	}

	// remeber current writemask option and depth function
	glGetBooleanv(GL_DEPTH_WRITEMASK, &previousState.writeDepthMask);
	glGetIntegerv(GL_DEPTH_FUNC, &previousState.depthFunc);

	// Store current viewport and FBO
	glGetIntegerv(GL_VIEWPORT, previousState.viewport);
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousState.FBO);

	// Set the eye matrices
	this->eyeProjectionMatrix = eyeProjectionMatrix;
	this->eyeViewMatrix = eyeViewMatrix;
	this->lightSpaceMatrix = lightSpaceMatrix;

	/// Load specific shader and pass level variables
	configRender();

	started = true;
}

void Eng::RenderPass::render(const std::shared_ptr<Eng::ListElement>& element) {
	if (!started) {
		std::cerr << "ERROR: Render pass not started. Call start() before render()." << std::endl;
		return;
	}

	perElementConfig(element);

	element->getNode()->render();
}

void Eng::RenderPass::stop() {
	if (started) {
		// Restore previous OpenGL blending state
		if (previousState.blendingEnabled) {
			glEnable(GL_BLEND);
			glBlendFunc(previousState.srcRGB, previousState.dstRGB);
		}
		else {
			glDisable(GL_BLEND);
		}

		// Restore previous depth mask option and depth function
		glDepthMask(previousState.writeDepthMask);
		glDepthFunc(previousState.depthFunc);

		// IMPORTANT: Restore the previous FBO and viewport
		glBindFramebuffer(GL_FRAMEBUFFER, previousState.FBO);
		glViewport(previousState.viewport[0], previousState.viewport[1], 
			previousState.viewport[2], previousState.viewport[3]);

		started = false;
	}
}