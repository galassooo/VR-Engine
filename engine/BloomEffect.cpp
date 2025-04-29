#include "engine.h"
#include <GL/glew.h>

Eng::BloomEffect::BloomEffect()
    : sceneColorTexture(0), sceneBrightTexture(0),
    quadVAO(0), quadVBO(0) {
    blurTextures[0] = 0;
    blurTextures[1] = 0;
}

Eng::BloomEffect::~BloomEffect() {
    if (sceneColorTexture)
        glDeleteTextures(1, &sceneColorTexture);
    if (sceneBrightTexture)
        glDeleteTextures(1, &sceneBrightTexture);
    if (blurTextures[0])
        glDeleteTextures(1, &blurTextures[0]);
    if (blurTextures[1])
        glDeleteTextures(1, &blurTextures[1]);
    if (quadVAO)
        glDeleteVertexArrays(1, &quadVAO);
    if (quadVBO)
        glDeleteBuffers(1, &quadVBO);
}

bool Eng::BloomEffect::init(int width, int height) {
    // Initialize the framebuffers
    if (!initFramebuffers(width, height)) {
        std::cerr << "ERROR: Failed to initialize bloom framebuffers" << std::endl;
        return false;
    }

    // Initialize the shaders
    if (!initShaders()) {
        std::cerr << "ERROR: Failed to initialize bloom shaders" << std::endl;
        return false;
    }

    // Initialize the quad for full-screen rendering
    float quadVertices[] = {
        // positions        // texture coordinates
        -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 0.0f, 1.0f, 0.0f,

        -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
         1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
         1.0f,  1.0f, 0.0f, 1.0f, 1.0f
    };

    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);

    initialized = true;
    std::cout << "Bloom effect initialized successfully!" << std::endl;
    return true;
}

bool Eng::BloomEffect::initFramebuffers(int width, int height) {
    // 1. FBO for the scene with two attachments: color and bright areas
    sceneFbo = std::make_shared<Eng::Fbo>();

    // Texture for the color
    glGenTextures(1, &sceneColorTexture);
    glBindTexture(GL_TEXTURE_2D, sceneColorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    sceneFbo->bindTexture(0, Eng::Fbo::BIND_COLORTEXTURE, sceneColorTexture, 0);

    // Texture for the bright areas
    glGenTextures(1, &sceneBrightTexture);
    glBindTexture(GL_TEXTURE_2D, sceneBrightTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    sceneFbo->bindTexture(1, Eng::Fbo::BIND_COLORTEXTURE, sceneBrightTexture, 1);

    // Renderbuffer for depth
    sceneFbo->bindRenderBuffer(2, Eng::Fbo::BIND_DEPTHBUFFER, width, height);

    if (!sceneFbo->isOk()) {
        std::cerr << "ERROR: Scene FBO setup failed" << std::endl;
        return false;
    }

    // 2. Two FBOs for the blur ping-pong
    for (int i = 0; i < 2; i++) {
        blurFbo[i] = std::make_shared<Eng::Fbo>();

        glGenTextures(1, &blurTextures[i]);
        glBindTexture(GL_TEXTURE_2D, blurTextures[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        blurFbo[i]->bindTexture(0, Eng::Fbo::BIND_COLORTEXTURE, blurTextures[i]);

        if (!blurFbo[i]->isOk()) {
            std::cerr << "ERROR: Blur FBO setup failed" << std::endl;
            return false;
        }
    }

    return true;
}

bool Eng::BloomEffect::initShaders() {
    // 1. Shader for extracting bright areas
    const char* brightFilterVS = R"(
    #version 440 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec2 aTexCoords;

    out vec2 TexCoords;

    void main() {
        TexCoords = aTexCoords;
        gl_Position = vec4(aPos, 1.0);
    }
    )";

    const char* brightFilterFS = R"(
   // Bright filter shader - extracts saturated primary colors
   #version 440 core
   out vec4 FragColor;
   in  vec2 TexCoords;

   uniform sampler2D sceneTex;

   // Thresholds can be exposed as uniforms if needed
   const float HIGH_THR = 0.85;   // How "full" the dominant channel must be
   const float LOW_THR  = 0.25;   // How low the other channels must be

   void main()
   {
       vec3 rgb = texture(sceneTex, TexCoords).rgb;

       bool isRed   = (rgb.r > HIGH_THR) && (rgb.g < LOW_THR) && (rgb.b < LOW_THR);
       bool isGreen = (rgb.g > HIGH_THR) && (rgb.r < LOW_THR) && (rgb.b < LOW_THR);
       bool isBlue  = (rgb.b > HIGH_THR) && (rgb.r < LOW_THR) && (rgb.g < LOW_THR);

       if (isRed || isGreen || isBlue)
           FragColor = vec4(rgb, 1.0);  // Pass to ping-pong for blur
       else
           FragColor = vec4(0.0);       // Discard: no bloom
   }
   )";

    std::shared_ptr<Eng::VertexShader> brightVS = std::make_shared<Eng::VertexShader>();
    brightVS->load(brightFilterVS);

    std::shared_ptr<Eng::FragmentShader> brightFS = std::make_shared<Eng::FragmentShader>();
    brightFS->load(brightFilterFS);

    brightFilterProgram = std::make_shared<Eng::Program>();
    brightFilterProgram->bindAttribute(0, "aPos");
    brightFilterProgram->bindAttribute(1, "aTexCoords");
    brightFilterProgram->bindSampler(0, "sceneTex");

    if (!brightFilterProgram->addShader(brightVS).addShader(brightFS).build()) {
        std::cerr << "ERROR: Failed to build bright filter program" << std::endl;
        return false;
    }

    // 2. Gaussian blur shader
    const char* blurVS = R"(
    #version 440 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec2 aTexCoords;

    out vec2 TexCoords;

    void main() {
        TexCoords = aTexCoords;
        gl_Position = vec4(aPos, 1.0);
    }
    )";

    const char* blurFS = R"(
    #version 440 core
    layout(location = 0) out vec4 FragColor;

    in vec2 TexCoords;

    layout(binding = 0) uniform sampler2D image;
    uniform bool horizontal;

    // Kernel with 12 samples for high-quality blur
    const int KERNEL = 12;
    const float weight[KERNEL] = float[](
        0.08, 0.075, 0.07, 0.065, 0.06, 0.055, 
        0.05, 0.045, 0.04, 0.035, 0.03, 0.025
    );

    void main()
    {
        // Calculate texel size (1/width, 1/height)
        vec2 off = 1.0 / vec2(textureSize(image, 0));

        // Sample center texel
        vec3 col = texture(image, TexCoords).rgb * weight[0];

        // Sample neighboring texels with weights
        for (int i = 1; i < KERNEL; ++i)
        {
            // Calculate offset based on direction
            vec2 delta = horizontal ? vec2(off.x * i, 0.0)
                                    : vec2(0.0, off.y * i);

            // Accumulate weighted samples in both directions
            col += texture(image, TexCoords + delta).rgb * weight[i];
            col += texture(image, TexCoords - delta).rgb * weight[i];
        }

        FragColor = vec4(col, 1.0);
    }
    )";

    std::shared_ptr<Eng::VertexShader> blurVShader = std::make_shared<Eng::VertexShader>();
    blurVShader->load(blurVS);

    std::shared_ptr<Eng::FragmentShader> blurFShader = std::make_shared<Eng::FragmentShader>();
    blurFShader->load(blurFS);

    blurProgram = std::make_shared<Eng::Program>();
    blurProgram->bindAttribute(0, "aPos");
    blurProgram->bindAttribute(1, "aTexCoords");
    blurProgram->bindSampler(0, "image");

    if (!blurProgram->addShader(blurVShader).addShader(blurFShader).build()) {
        std::cerr << "ERROR: Failed to build blur program" << std::endl;
        return false;
    }

    // 3. Final combination shader
    const char* bloomFinalVS = R"(
    #version 440 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec2 aTexCoords;

    out vec2 TexCoords;

    void main() {
        TexCoords = aTexCoords;
        gl_Position = vec4(aPos, 1.0);
    }
    )";

    const char* bloomFinalFS = R"(
    #version 440 core
    out vec4 FragColor;
    in  vec2 TexCoords;

    layout(binding = 0) uniform sampler2D sceneTex;
    layout(binding = 1) uniform sampler2D bloomTex;

    uniform float bloomIntensity = 0.3;  // Bloom strength
    uniform bool applyTonemap = true;    // HDR→LDR toggle

    // ACES Filmic Tone Mapping Operator
    vec3 ACESFilm(vec3 x) {
        float a = 2.51f;
        float b = 0.03f;
        float c = 2.43f;
        float d = 0.59f;
        float e = 0.14f;
        return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0, 1.0);
    }

    void main() {
        // Combine scene with bloom effect
        vec3 hdr = texture(sceneTex, TexCoords).rgb +
                  texture(bloomTex, TexCoords).rgb * bloomIntensity;

        if (applyTonemap) {
            // Use fixed exposure for ACES tone mapping
            float exposureValue = 0.7;
            
            // Apply ACES Filmic tone mapping
            hdr = ACESFilm(hdr * exposureValue);
        }

        FragColor = vec4(hdr, 1.0);
    }
    )";

    std::shared_ptr<Eng::VertexShader> bloomFinalVShader = std::make_shared<Eng::VertexShader>();
    bloomFinalVShader->load(bloomFinalVS);

    std::shared_ptr<Eng::FragmentShader> bloomFinalFShader = std::make_shared<Eng::FragmentShader>();
    bloomFinalFShader->load(bloomFinalFS);

    bloomFinalProgram = std::make_shared<Eng::Program>();
    bloomFinalProgram->bindAttribute(0, "aPos");
    bloomFinalProgram->bindAttribute(1, "aTexCoords");
    bloomFinalProgram->bindSampler(0, "sceneTex");
    bloomFinalProgram->bindSampler(1, "bloomTex");

    if (!bloomFinalProgram->addShader(bloomFinalVShader).addShader(bloomFinalFShader).build()) {
        std::cerr << "ERROR: Failed to build bloom final program" << std::endl;
        return false;
    }

    return true;
}

void Eng::BloomEffect::analyzeTextureHDRContent(unsigned int textureID, int width, int height) {
    if (textureID == 0) {
        std::cout << "ERROR: Invalid texture ID for analysis" << std::endl;
        return;
    }

    // Create temporary framebuffer to read the texture
    GLuint tempFBO;
    glGenFramebuffers(1, &tempFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, tempFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureID, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR: Temporary FBO not complete for texture analysis" << std::endl;
        glDeleteFramebuffers(1, &tempFBO);
        return;
    }

    // Allocate buffer for texture data (float for HDR support)
    std::vector<float> buffer(width * height * 4); // RGBA = 4 components

    // Read texture data
    glReadPixels(0, 0, width, height, GL_RGBA, GL_FLOAT, buffer.data());

    // Statistics
    float minValue = FLT_MAX;
    float maxValue = -FLT_MAX;
    float sumValue = 0.0f;
    int hdrPixelCount = 0;
    int totalPixels = width * height;

    // Analyze values in RGB components (ignoring Alpha)
    for (int i = 0; i < totalPixels; i++) {
        for (int c = 0; c < 3; c++) { // RGB components only
            float value = buffer[i * 4 + c];

            minValue = std::min(minValue, value);
            maxValue = std::max(maxValue, value);
            sumValue += value;

            // Count pixels with HDR values (>1.0) in any component
            if (value > 1.0f) {
                hdrPixelCount++;
                break; // Count only once per pixel
            }
        }
    }

    float avgValue = sumValue / (totalPixels * 3.0f); // Average across all RGB components

    // Print results
    std::cout << "===== HDR TEXTURE ANALYSIS =====" << std::endl;
    std::cout << "Texture size: " << width << "x" << height << " (" << totalPixels << " pixels)" << std::endl;
    std::cout << "Min value: " << minValue << std::endl;
    std::cout << "Max value: " << maxValue << std::endl;
    std::cout << "Avg value: " << avgValue << std::endl;
    std::cout << "HDR pixels (>1.0): " << hdrPixelCount << " ("
        << (hdrPixelCount * 100.0f / totalPixels) << "%)" << std::endl;
    std::cout << "=========================================" << std::endl;

    // Cleanup
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &tempFBO);
}

void Eng::BloomEffect::beginSceneCapture() {
    if (!initialized) return;

    // Activate the scene framebuffer to capture the 3D scene
    sceneFbo->render();

    // Configure attachments for rendering
    GLenum drawBuffers[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, drawBuffers);

    // Clear buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Eng::BloomEffect::endSceneCapture() {
    if (!initialized) return;

    // Extract bright areas
    extractBrightAreas();

    // Apply Gaussian blur
    applyGaussianBlur();

    // Combine results
    combineResults();
}

void Eng::BloomEffect::extractBrightAreas() {
    // Use the first ping-pong buffer for extraction
    blurFbo[0]->render();
    glClear(GL_COLOR_BUFFER_BIT);

    brightFilterProgram->render();

    // Set scene texture
    int sceneTexLoc = brightFilterProgram->getParamLocation("sceneTex");
    brightFilterProgram->setInt(sceneTexLoc, 0);

    int thresholdLoc = brightFilterProgram->getParamLocation("threshold");
    // Threshold parameter can be exposed if needed
    brightFilterProgram->setFloat(thresholdLoc, bloomThreshold);

    // Bind scene color texture as input for bright pass extraction
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sceneColorTexture);

    // Render fullscreen quad
    renderQuad();
}

void Eng::BloomEffect::applyGaussianBlur() {
    blurProgram->render();

    int imageLoc = blurProgram->getParamLocation("image");
    int horizontalLoc = blurProgram->getParamLocation("horizontal");

    // Apply repeated Gaussian blur with ping-pong technique
    // Total passes = blurPasses * 2 (alternating horizontal/vertical)
    bool horizontal = true;
    for (int i = 0; i < blurPasses * 2; i++) {
        blurFbo[horizontal ? 1 : 0]->render();
        glClear(GL_COLOR_BUFFER_BIT);

        // Set shader parameters
        blurProgram->setInt(imageLoc, 0);
        blurProgram->setInt(horizontalLoc, horizontal ? 1 : 0);

        // Use texture from previous pass
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, (i == 0) ? blurTextures[0] : blurTextures[!horizontal]);

        // Render fullscreen quad
        renderQuad();

        // Toggle direction for next pass
        horizontal = !horizontal;
    }
}

void Eng::BloomEffect::combineResults() {
    // Restore default framebuffer
    Eng::Fbo::disable();

    // Clear screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Use final combination shader
    bloomFinalProgram->render();

    // Set textures and bloom intensity
    int sceneTexLoc = bloomFinalProgram->getParamLocation("sceneTex");
    int bloomTexLoc = bloomFinalProgram->getParamLocation("bloomTex");
    int intensityLoc = bloomFinalProgram->getParamLocation("bloomIntensity");

    bloomFinalProgram->setInt(sceneTexLoc, 0);
    bloomFinalProgram->setInt(bloomTexLoc, 1);
    bloomFinalProgram->setFloat(intensityLoc, bloomIntensity);

    // Bind textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sceneColorTexture); // Original scene color
    glActiveTexture(GL_TEXTURE1);
    // Use the last texture from the blur ping-pong
    glBindTexture(GL_TEXTURE_2D, blurTextures[!((blurPasses * 2) % 2)]);

    // Render fullscreen quad
    renderQuad();
}

void Eng::BloomEffect::renderQuad() {
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

bool Eng::BloomEffect::isInitialized() const {
    return initialized;
}

bool Eng::BloomEffect::processExternalTexture(unsigned int inputTexture, unsigned int outputTexture, int width, int height) {
    if (!initialized) return false;

    // Analyze input texture (optional - for debugging)
    // std::cout << "Analyzing input texture:" << std::endl;
    // analyzeTextureHDRContent(inputTexture, width, height);

    // 1. Extract bright areas from input texture
    blurFbo[0]->render();
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT);

    brightFilterProgram->render();
    int sceneTexLoc = brightFilterProgram->getParamLocation("sceneTex");
    brightFilterProgram->setInt(sceneTexLoc, 0);

    int thresholdLoc = brightFilterProgram->getParamLocation("threshold");
    brightFilterProgram->setFloat(thresholdLoc, bloomThreshold);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputTexture);
    renderQuad();

    // 2. Apply Gaussian blur to the bright areas
    applyGaussianBlur();

    // 3. Combine original scene with bloom effect
    GLuint tempFbo;
    glGenFramebuffers(1, &tempFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, tempFbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outputTexture, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR: Temporary FBO for bloom is not complete" << std::endl;
        glDeleteFramebuffers(1, &tempFbo);
        return false;
    }

    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT);

    bloomFinalProgram->render();

    int sceneTexLocation = bloomFinalProgram->getParamLocation("sceneTex");
    int bloomTexLocation = bloomFinalProgram->getParamLocation("bloomTex");
    int intensityLocation = bloomFinalProgram->getParamLocation("bloomIntensity");

    bloomFinalProgram->setInt(sceneTexLocation, 0);
    bloomFinalProgram->setInt(bloomTexLocation, 1);
    bloomFinalProgram->setFloat(intensityLocation, bloomIntensity);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputTexture); // Original external texture

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, blurTextures[!((blurPasses * 2) % 2)]); // Final blurred texture

    renderQuad();

    // Analyze output texture (optional - for debugging)
    // std::cout << "Analyzing output texture:" << std::endl;
    // analyzeTextureHDRContent(outputTexture, width, height);

    // Cleanup
    glDeleteFramebuffers(1, &tempFbo);

    return true; // Indicate success
}

void Eng::BloomEffect::applyToTexture(unsigned int inputTexture, unsigned int outputTexture, int width, int height) {
    if (!initialized) return;

    // Try to apply bloom effect
    if (!processExternalTexture(inputTexture, outputTexture, width, height)) {
        std::cout << "WARNING: Bloom processing failed, falling back to direct copy" << std::endl;

        // If bloom fails, simply copy input texture to output texture
        GLuint tempFbo;
        glGenFramebuffers(1, &tempFbo);
        glBindFramebuffer(GL_FRAMEBUFFER, tempFbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outputTexture, 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE) {
            glViewport(0, 0, width, height);
            glClear(GL_COLOR_BUFFER_BIT);

            // Simple pass-through shader for copying the texture
            static const char* basicVS = R"(
            #version 440 core
            layout (location = 0) in vec3 aPos;
            layout (location = 1) in vec2 aTexCoords;
            out vec2 TexCoords;
            void main() {
                TexCoords = aTexCoords;
                gl_Position = vec4(aPos, 1.0);
            }
            )";

            static const char* basicFS = R"(
            #version 440 core
            out vec4 FragColor;
            in vec2 TexCoords;
            uniform sampler2D inputTex;
            void main() {
                FragColor = texture(inputTex, TexCoords);
            }
            )";

            // Create a one-time shader for copying the texture
            std::shared_ptr<Eng::VertexShader> vs = std::make_shared<Eng::VertexShader>();
            vs->load(basicVS);

            std::shared_ptr<Eng::FragmentShader> fs = std::make_shared<Eng::FragmentShader>();
            fs->load(basicFS);

            std::shared_ptr<Eng::Program> copyProgram = std::make_shared<Eng::Program>();
            copyProgram->bindAttribute(0, "aPos");
            copyProgram->bindAttribute(1, "aTexCoords");
            copyProgram->bindSampler(0, "inputTex");

            if (copyProgram->addShader(vs).addShader(fs).build()) {
                copyProgram->render();
                copyProgram->setInt(copyProgram->getParamLocation("inputTex"), 0);

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, inputTexture);

                renderQuad();
            }
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &tempFbo);
    }

    // Reset to default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}