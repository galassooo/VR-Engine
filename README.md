# Graphics Engine

A modern OpenGL-based 3D graphics engine with support for advanced rendering features, including deferred rendering, post-processing effects, stereoscopic VR rendering, and physically-based lighting.

## Overview

This graphics engine provides a comprehensive framework for real-time 3D rendering applications. Built on top of OpenGL 4.4, it implements a flexible scene graph hierarchy, deferred rendering pipeline, and shader-based material system. The engine includes support for VR rendering through OpenVR/SteamVR integration.

## Features

### Core Features
- Scene graph architecture with hierarchical transformations
- Deferred rendering pipeline for efficient lighting
- Material system with customizable shaders
- Texture loading and management via FreeImage
- Camera system with perspective and orthographic projections

### Advanced Rendering
- **Post-processing Framework**:
  - Bloom effect for enhanced visual quality
  - Extensible post-processor architecture for custom effects
- **Lighting System**:
  - Directional lights with shadow mapping
  - Point lights with appropriate attenuation
  - Spot lights with configurable cone and falloff
- **Environment Features**:
  - HDR skybox loading and rendering
  - Global illumination approximation from skybox

### VR Support
- OpenVR/SteamVR integration for HMD rendering
- Stereoscopic rendering with appropriate eye separation
- VR controller tracking and interaction framework

## Architecture

The engine is structured around several key components:

### Scene Graph
- `Node`: Base class for all scene objects with hierarchical transformations
- `Mesh`: Renderable geometry with materials
- `Camera`: View and projection definition (Perspective/Orthographic)
- `Light`: Abstract light sources in the scene

### Rendering
- `RenderPipeline`: Manages multi-pass rendering workflow
- `ShaderManager`: Handles shader compilation, binding, and uniform management
- `PostProcessorManager`: Coordinates post-processing effects chain
- `BloomEffect`: Implements HDR bloom for enhanced lighting visualization

### Resources
- `Texture`: 2D texture loading and binding
- `Material`: Surface appearance definition
- `Program`: OpenGL shader program wrapper
- `Builder`: Factory for creating and uploading meshes

## Implementation Details

### Rendering Pipeline
The engine uses a deferred rendering approach:

1. Base color pass renders materials to framebuffers
2. For each light, a separate pass accumulates illumination
3. Shadow maps are generated for directional lights
4. Post-processing effects are applied in sequence
5. Final composition renders to screen or VR headset

### Stereoscopic VR
For VR rendering, the pipeline:

1. Gets tracking data from OpenVR
2. Renders the scene twice (once per eye) with appropriate view matrices
3. Applies post-processing effects to each eye's render
4. Submits frames to the VR runtime

### Material System
Materials define surface properties through:
- Diffuse/ambient/specular/emission colors
- Texture maps
- Shininess/smoothness
- Custom shader support (e.g., HolographicMaterial)

## Getting Started

### Prerequisites
- OpenGL 4.4 compatible GPU
- FreeGLUT and GLEW for OpenGL context and extension handling
- FreeImage for texture loading
- GLM for mathematics
- OpenVR SDK (for VR support)

### Basic Usage

```cpp
#include "Engine.h"

int main() {
    // Initialize the engine
    auto& engine = Eng::Base::getInstance();
    engine.init();

    // Load a scene
    engine.loadScene("my_scene.ovo");

    // Set up a camera
    auto camera = std::make_shared<Eng::PerspectiveCamera>(
        60.0f, engine.getWindowAspectRatio(), 0.1f, 1000.0f);
    engine.SetActiveCamera(camera);

    // Add post-processing if desired
    auto bloom = std::make_shared<Eng::BloomEffect>();
    bloom->init(APP_WINDOWSIZEX, APP_WINDOWSIZEY);
    engine.addPostProcessor(bloom);

    // Start the render loop
    engine.run();

    // Cleanup
    engine.free();
    return 0;
}
```

## Advanced Features

### Custom Post-processors
Create your own post-processing effects by extending the `PostProcessor` class:

```cpp
class MyEffect : public Eng::PostProcessor {
public:
    bool init(int width, int height) override {
        // Initialize shaders, framebuffers, etc.
    }

    void applyEffect(unsigned int inputTexture, unsigned int outputTexture, 
                     int width, int height) override {
        // Apply custom processing
    }
};
```

### Custom Materials
Extend the `Material` class to create specialized material types:

```cpp
class CustomMaterial : public Eng::Material {
public:
    void render() override {
        // Set up custom shader parameters
    }
};
```

## Contributors

- Kevin Alexander Quarenghi Escobar
- Martina Galasso
- Lorenzo Forestieri

## License

This project is licensed under the MIT License - see the LICENSE file for details.
