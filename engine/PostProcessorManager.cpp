#include "Engine.h"
#include <GL/glew.h>

Eng::PostProcessorManager& Eng::PostProcessorManager::getInstance() {
    static PostProcessorManager instance;
    return instance;
}

bool Eng::PostProcessorManager::initializeAll(int width, int height) {
    if (width <= 0 || height <= 0) {
        std::cerr << "ERROR: Invalid dimensions for post-processor initialization" << std::endl;
        return false;
    }

    currentWidth = width;
    currentHeight = height;
    bool success = true;

    for (auto& processor : postProcessors) {
        if (!processor->init(width, height)) {
            std::cerr << "ERROR: Failed to initialize post-processor: " << processor->getName() << std::endl;
            success = false;
        }
    }

    // Initialize the temporary texture
    if (success) {
        success = ensureTempTexture(width, height);
    }

    return success;
}

bool Eng::PostProcessorManager::addPostProcessor(std::shared_ptr<PostProcessor> postProcessor) {
    if (!postProcessor) {
        std::cerr << "ERROR: Cannot add null post-processor" << std::endl;
        return false;
    }

    // Check if a post-processor with the same name already exists
    for (const auto& processor : postProcessors) {
        if (processor->getName() == postProcessor->getName()) {
            std::cerr << "ERROR: Post-processor with name '" << postProcessor->getName() << "' already exists" << std::endl;
            return false;
        }
    }

    // Initialize the post-processor if dimensions are known
    if (currentWidth > 0 && currentHeight > 0) {
        if (!postProcessor->init(currentWidth, currentHeight)) {
            std::cerr << "ERROR: Failed to initialize post-processor: " << postProcessor->getName() << std::endl;
            return false;
        }
    }

    postProcessors.push_back(postProcessor);
    return true;
}

bool Eng::PostProcessorManager::removePostProcessor(const std::string& name) {
    auto it = std::find_if(postProcessors.begin(), postProcessors.end(),
        [&name](const std::shared_ptr<PostProcessor>& processor) {
            return processor->getName() == name;
        });

    if (it != postProcessors.end()) {
        postProcessors.erase(it);
        return true;
    }

    return false;
}

std::shared_ptr<Eng::PostProcessor> Eng::PostProcessorManager::getPostProcessor(const std::string& name) {
    for (const auto& processor : postProcessors) {
        if (processor->getName() == name) {
            return processor;
        }
    }

    return nullptr;
}

void Eng::PostProcessorManager::applyPostProcessing(unsigned int inputTexture, unsigned int outputTexture, int width, int height) {
    if (!postProcessingEnabled || postProcessors.empty()) {
        // If post-processing is disabled or there are no processors, just copy the input to output
        GLuint tempFbo;
        glGenFramebuffers(1, &tempFbo);
        glBindFramebuffer(GL_FRAMEBUFFER, tempFbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outputTexture, 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE) {
            // Copy using a simple blit
            GLuint sourceFbo;
            glGenFramebuffers(1, &sourceFbo);
            glBindFramebuffer(GL_READ_FRAMEBUFFER, sourceFbo);
            glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, inputTexture, 0);

            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, tempFbo);
            glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

            glDeleteFramebuffers(1, &sourceFbo);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &tempFbo);
        return;
    }

    // Ensure temp texture exists and has correct size
    if (!ensureTempTexture(width, height)) {
        std::cerr << "ERROR: Failed to create temporary texture for post-processing" << std::endl;
        return;
    }

    // Handle special case of single post-processor
    if (postProcessors.size() == 1) {
        postProcessors[0]->applyEffect(inputTexture, outputTexture, width, height);
        return;
    }

    // Apply post-processors in sequence, using temporary textures for intermediate results
    unsigned int currentInput = inputTexture;
    unsigned int currentOutput = tempTexture;

    for (size_t i = 0; i < postProcessors.size(); i++) {
        // For the last processor, output to the final output texture
        if (i == postProcessors.size() - 1) {
            currentOutput = outputTexture;
        }

        postProcessors[i]->applyEffect(currentInput, currentOutput, width, height);

        // Update input for next processor
        if (i < postProcessors.size() - 1) {
            // Swap input and output for next pass
            std::swap(currentInput, currentOutput);
        }
    }
}

bool Eng::PostProcessorManager::isPostProcessingEnabled() const {
    return postProcessingEnabled;
}

void Eng::PostProcessorManager::setPostProcessingEnabled(bool enabled) {
    postProcessingEnabled = enabled;
}

size_t Eng::PostProcessorManager::getPostProcessorCount() const {
    return postProcessors.size();
}

bool Eng::PostProcessorManager::ensureTempTexture(int width, int height) {
    // Check if we need to create or resize the texture
    if (tempTexture == 0 || width != currentWidth || height != currentHeight) {
        // Delete old texture if it exists
        if (tempTexture != 0) {
            glDeleteTextures(1, &tempTexture);
        }

        // Create a new texture
        glGenTextures(1, &tempTexture);
        glBindTexture(GL_TEXTURE_2D, tempTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // Update current dimensions
        currentWidth = width;
        currentHeight = height;

        // Check if texture creation succeeded
        if (tempTexture == 0) {
            std::cerr << "ERROR: Failed to create temporary texture for post-processing" << std::endl;
            return false;
        }
    }

    return true;
}