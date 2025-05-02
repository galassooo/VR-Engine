#pragma once

/**
 * @class BloomEffect
 * @brief Implements a post-processing bloom effect to simulate glow around bright areas.
 *
 * This class handles the main steps of the bloom effect:
 * 1. Extracts bright areas from the scene
 * 2. Applies a Gaussian blur to these areas
 * 3. Combines the blurred brightness with the original scene
 */
class ENG_API BloomEffect : public Eng::PostProcessor {
public:
    BloomEffect();
    ~BloomEffect();

    /**
     * @brief Initializes the bloom effect with the specified dimensions
     * @param width Width of the effect's render target
     * @param height Height of the effect's render target
     * @return true if initialization was successful, false otherwise
     */
    bool init(int width, int height) override;

    /**
     * @brief Starts rendering the scene to the internal bloom framebuffer
     */
    void beginSceneCapture();

    /**
     * @brief Finalizes the bloom processing and renders the result to the screen
     */
    void endSceneCapture();

    /**
     * @brief Checks if the bloom effect has been initialized
     * @return true if the effect is initialized, false otherwise
     */
    bool isInitialized() const override;

    /**
     * @brief Processes an external input texture and outputs the bloom result to another texture
     * @param inputTexture OpenGL texture ID containing the input image
     * @param outputTexture OpenGL texture ID where the result will be written
     * @param width Width of the textures
     * @param height Height of the textures
     * @return true if processing was successful, false otherwise
     */
    bool processExternalTexture(unsigned int inputTexture, unsigned int outputTexture, int width, int height);

    /**
     * @brief Applies bloom to an input texture and renders the result to an output texture
     * With error handling and fallback to direct copy if bloom fails
     * @param inputTexture OpenGL texture ID containing the input image
     * @param outputTexture OpenGL texture ID where the result will be written
     * @param width Width of the textures
     * @param height Height of the textures
     */
    void applyEffect(unsigned int inputTexture, unsigned int outputTexture, int width, int height) override;

    /**
     * @brief Sets a parameter for the bloom effect
     * @param name Parameter name (e.g., "intensity", "threshold", "passes")
     * @param value Parameter value as a float
     */
    void setParameter(const std::string& name, float value) override;

    /**
     * @brief Gets the name of this post-processor
     * @return The name "BloomEffect"
     */
    std::string getName() const override;

    /**
     * @brief Analyzes the HDR content of a texture for debugging and diagnostics
     * @param textureID OpenGL texture ID to analyze
     * @param width Width of the texture
     * @param height Height of the texture
     */
    void analyzeTextureHDRContent(unsigned int textureID, int width, int height);

private:
    /** Flag indicating if the effect has been initialized */
    bool initialized = false;

    /** Bloom effect parameters */
    float bloomThreshold = 0.1f;  // Threshold to extract bright areas
    float bloomIntensity = 0.3f;  // Intensity of the bloom effect
    int blurPasses = 3;           // Number of Gaussian blur passes

    /** FBO and textures for scene capture */
    std::shared_ptr<Eng::Fbo> sceneFbo;
    unsigned int sceneColorTexture;
    unsigned int sceneBrightTexture;

    /** FBOs and textures used for two-pass Gaussian blur */
    std::shared_ptr<Eng::Fbo> blurFbo[2];
    unsigned int blurTextures[2];

    /** Shaders for different bloom stages */
    std::shared_ptr<Eng::Program> brightFilterProgram;
    std::shared_ptr<Eng::Program> blurProgram;
    std::shared_ptr<Eng::Program> bloomFinalProgram;

    /** General purpose shaders */
    std::shared_ptr<Eng::Program> copyProgram;

    /** Geometry for full-screen rendering */
    unsigned int quadVAO;
    unsigned int quadVBO;

    /**
     * @brief Initializes the framebuffers used for bloom processing
     * @param width Width of the framebuffers
     * @param height Height of the framebuffers
     * @return true if initialization was successful, false otherwise
     */
    bool initFramebuffers(int width, int height);

    /**
     * @brief Initializes the shaders used for bloom processing
     * @return true if initialization was successful, false otherwise
     */
    bool initShaders();

    /**
     * @brief Renders a full-screen quad for post-processing
     */
    void renderQuad();

    /**
     * @brief Extracts high luminance areas from the scene
     */
    void extractBrightAreas();

    /**
     * @brief Applies Gaussian blur to the extracted brightness
     */
    void applyGaussianBlur();

    /**
     * @brief Combines the blurred brightness with the original scene
     */
    void combineResults();
};