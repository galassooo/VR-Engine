#pragma once

/**
 * @enum RenderLayer
 * @brief Defines the rendering order for objects in the scene.
 */
enum class RenderLayer {
    Lights = 0,        ///< Lights are rendered first.
    Opaque = 1,        ///< Opaque objects are rendered after lights.
    Transparent = 2    ///< Transparent objects are rendered last.
};