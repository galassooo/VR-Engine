#pragma once

class ENG_API BloomRenderer {
public:
    BloomRenderer();
    ~BloomRenderer();

    bool init(int width, int height);
    void render(unsigned int inputTexture);
    unsigned int getBloomTexture() const { return bloomTexture; }

private:
    // FBOs for the bloom effect
    std::shared_ptr<Eng::Fbo> brightPassFbo;
    std::shared_ptr<Eng::Fbo> horizontalBlurFbo;
    std::shared_ptr<Eng::Fbo> verticalBlurFbo;

    // Textures
    unsigned int brightPassTexture;
    unsigned int horizontalBlurTexture;
    unsigned int bloomTexture;  // Final bloom texture

    // Shader programs
    std::shared_ptr<Eng::Program> brightPassProgram;
    std::shared_ptr<Eng::Program> blurProgram;
    std::shared_ptr<Eng::Program> finalBlendProgram;

    // VAO and VBO for a fullscreen quad
    unsigned int quadVAO;
    unsigned int quadVBO;

    // Screen dimensions
    int width;
    int height;

    void setupQuad();
    void renderQuad();
    bool setupShaders();
};