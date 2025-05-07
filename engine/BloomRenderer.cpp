#include "engine.h"
#include <GL/glew.h>

// Fullscreen quad vertices
const float quadVertices[] = {
    // positions        // texture coords
    -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
    -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
     1.0f, -1.0f, 0.0f, 1.0f, 0.0f,

    -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
     1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
     1.0f,  1.0f, 0.0f, 1.0f, 1.0f
};

Eng::BloomRenderer::BloomRenderer()
    : brightPassTexture(0), horizontalBlurTexture(0), bloomTexture(0),
    quadVAO(0), quadVBO(0), width(0), height(0) {
}

Eng::BloomRenderer::~BloomRenderer() {
    if (brightPassTexture) glDeleteTextures(1, &brightPassTexture);
    if (horizontalBlurTexture) glDeleteTextures(1, &horizontalBlurTexture);
    if (bloomTexture) glDeleteTextures(1, &bloomTexture);
    if (quadVAO) glDeleteVertexArrays(1, &quadVAO);
    if (quadVBO) glDeleteBuffers(1, &quadVBO);
}

bool Eng::BloomRenderer::init(int width, int height) {
    this->width = width;
    this->height = height;

    // Set up the fullscreen quad
    setupQuad();

    // Set up shaders
    if (!setupShaders()) {
        std::cerr << "Failed to set up bloom shaders" << std::endl;
        return false;
    }

    // Create bright pass FBO
    brightPassFbo = std::make_shared<Eng::Fbo>();
    glGenTextures(1, &brightPassTexture);
    glBindTexture(GL_TEXTURE_2D, brightPassTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    brightPassFbo->bindTexture(0, Eng::Fbo::BIND_COLORTEXTURE, brightPassTexture);
    brightPassFbo->bindRenderBuffer(1, Eng::Fbo::BIND_DEPTHBUFFER, width, height);

    // Create horizontal blur FBO
    horizontalBlurFbo = std::make_shared<Eng::Fbo>();
    glGenTextures(1, &horizontalBlurTexture);
    glBindTexture(GL_TEXTURE_2D, horizontalBlurTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    horizontalBlurFbo->bindTexture(0, Eng::Fbo::BIND_COLORTEXTURE, horizontalBlurTexture);

    // Create vertical blur FBO
    verticalBlurFbo = std::make_shared<Eng::Fbo>();
    glGenTextures(1, &bloomTexture);
    glBindTexture(GL_TEXTURE_2D, bloomTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    verticalBlurFbo->bindTexture(0, Eng::Fbo::BIND_COLORTEXTURE, bloomTexture);

    return true;
}

void Eng::BloomRenderer::setupQuad() {
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);

    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Texture coordinate attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void Eng::BloomRenderer::renderQuad() {
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

bool Eng::BloomRenderer::setupShaders() {
    // Bright pass shader for extracting bright areas
    const char* brightPassVertexShaderSrc = R"(
        #version 440 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec2 aTexCoords;
        
        out vec2 TexCoords;
        
        void main() {
            TexCoords = aTexCoords;
            gl_Position = vec4(aPos, 1.0);
        }
    )";

    const char* brightPassFragmentShaderSrc = R"(
    #version 440 core
    out vec4 FragColor;
    in vec2 TexCoords;
    
    uniform sampler2D sceneTexture;
    uniform float threshold;
    
    void main() {
        vec4 color = texture(sceneTexture, TexCoords);
        
        // Calcola la luminanza con pesi più precisi
        float brightness = dot(color.rgb, vec3(0.2126, 0.7152, 0.0722));
        
        // Estrai solo le aree molto luminose con soglia più bassa
        if(brightness > threshold) {
            // Soft threshold per un bloom più naturale
            float softness = 0.1;
            float contribution = smoothstep(threshold, threshold + softness, brightness);
            FragColor = vec4(color.rgb * contribution, 1.0);
        } else {
            FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        }
    }
)";

    // Gaussian blur shader
    const char* blurVertexShaderSrc = R"(
        #version 440 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec2 aTexCoords;
        
        out vec2 TexCoords;
        
        void main() {
            TexCoords = aTexCoords;
            gl_Position = vec4(aPos, 1.0);
        }
    )";

    const char* blurFragmentShaderSrc = R"(
        #version 440 core
        out vec4 FragColor;
        in vec2 TexCoords;
        
        uniform sampler2D image;
        uniform bool horizontal;
        uniform float weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);
        
        void main() {
            vec2 tex_offset = 1.0 / textureSize(image, 0);
            vec3 result = texture(image, TexCoords).rgb * weight[0];
            
            if(horizontal) {
                for(int i = 1; i < 5; ++i) {
                    result += texture(image, TexCoords + vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
                    result += texture(image, TexCoords - vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
                }
            } else {
                for(int i = 1; i < 5; ++i) {
                    result += texture(image, TexCoords + vec2(0.0, tex_offset.y * i)).rgb * weight[i];
                    result += texture(image, TexCoords - vec2(0.0, tex_offset.y * i)).rgb * weight[i];
                }
            }
            
            FragColor = vec4(result, 1.0);
        }
    )";

    // Compile shaders
    std::shared_ptr<Eng::VertexShader> brightPassVS = std::make_shared<Eng::VertexShader>();
    std::shared_ptr<Eng::FragmentShader> brightPassFS = std::make_shared<Eng::FragmentShader>();

    std::shared_ptr<Eng::VertexShader> blurVS = std::make_shared<Eng::VertexShader>();
    std::shared_ptr<Eng::FragmentShader> blurFS = std::make_shared<Eng::FragmentShader>();

    if (!brightPassVS->load(brightPassVertexShaderSrc) ||
        !brightPassFS->load(brightPassFragmentShaderSrc) ||
        !blurVS->load(blurVertexShaderSrc) ||
        !blurFS->load(blurFragmentShaderSrc)) {
        std::cerr << "Failed to compile bloom shaders" << std::endl;
        return false;
    }

    // Create shader programs
    brightPassProgram = std::make_shared<Eng::Program>();
    brightPassProgram->addShader(brightPassVS);
    brightPassProgram->addShader(brightPassFS);
    brightPassProgram->bindAttribute(0, "aPos");
    brightPassProgram->bindAttribute(1, "aTexCoords");
    brightPassProgram->bindSampler(0, "sceneTexture");

    if (!brightPassProgram->build()) {
        std::cerr << "Failed to build bright pass program" << std::endl;
        return false;
    }

    blurProgram = std::make_shared<Eng::Program>();
    blurProgram->addShader(blurVS);
    blurProgram->addShader(blurFS);
    blurProgram->bindAttribute(0, "aPos");
    blurProgram->bindAttribute(1, "aTexCoords");
    blurProgram->bindSampler(0, "image");

    if (!blurProgram->build()) {
        std::cerr << "Failed to build blur program" << std::endl;
        return false;
    }

    return true;
}

void Eng::BloomRenderer::render(unsigned int inputTexture) {
    // 1. Estrai le aree luminose
    brightPassFbo->render();
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT);

    brightPassProgram->render();

    // Collega la texture della scena
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputTexture);

    // Imposta la soglia per il bright pass
    int thresholdLoc = brightPassProgram->getParamLocation("threshold");
    brightPassProgram->setFloat(thresholdLoc, 0.5f); // Regola la soglia in base alle tue necessità

    renderQuad();

    // 2. Applica il blur orizzontale Gaussiano
    horizontalBlurFbo->render();
    glClear(GL_COLOR_BUFFER_BIT);

    blurProgram->render();

    // Collega la texture del bright pass
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, brightPassTexture);

    // Imposta il blur orizzontale
    int horizontalLoc = blurProgram->getParamLocation("horizontal");
    blurProgram->setInt(horizontalLoc, 1); // true per il passaggio orizzontale

    renderQuad();

    // 3. Applica il blur verticale Gaussiano
    verticalBlurFbo->render();
    glClear(GL_COLOR_BUFFER_BIT);

    // Collega la texture del blur orizzontale
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, horizontalBlurTexture);

    // Imposta il blur verticale
    blurProgram->setInt(horizontalLoc, 0); // false per il passaggio verticale

    renderQuad();

    // Nota: NON ritorniamo al framebuffer predefinito qui
    // Lasceremo che sia la funzione di rendering principale a gestire la composizione
}