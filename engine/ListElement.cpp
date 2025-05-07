#include "Engine.h"

/**
 * @brief Constructs a ListElement object.
 *
 * This constructor initializes a ListElement with a reference to a scene graph node and its transformation matrix in world space.
 * It also determines the render layer of the node based on its type and material properties.
 *
 * @param node A shared pointer to the node being added to the render list.
 * @param worldCoordinates The transformation matrix in world space for the node.
 */
Eng::ListElement::ListElement(const std::shared_ptr<Eng::Node> &node,
                              const glm::mat4 &worldCoordinates): node{node}, worldCoordinates{worldCoordinates} {
   if (std::dynamic_pointer_cast<Eng::Light>(node)) {
      layer = RenderLayer::Lights;
   } else if (auto mesh = std::dynamic_pointer_cast<Eng::Mesh>(node)) {
      auto material = mesh->getMaterial();
      layer = (material && material->getAlpha() < 1.0f)
                 ? RenderLayer::Transparent
                 : RenderLayer::Opaque;
   } else {
      layer = RenderLayer::Opaque; // Default to opaque
   }
}

/**
 * @brief Retrieves the node associated with this ListElement.
 *
 * @return std::shared_ptr<Eng::Node> A shared pointer to the node.
 */
std::shared_ptr<Eng::Node> Eng::ListElement::getNode() const {
   return node;
}

/**
 * @brief Retrieves the world coordinates of this ListElement.
 *
 * The world coordinates represent the transformation matrix of the node in world space,
 * including any parent transformations.
 *
 * @return glm::mat4 The world transformation matrix.
 */
glm::mat4 Eng::ListElement::getWorldCoordinates() const {
   return worldCoordinates;
}

/**
 * @brief Retrieves the render layer of this ListElement.
 * @return Eng::RenderLayer The render layer (Lights, Opaque, Transparent).
 */
Eng::RenderLayer Eng::ListElement::getLayer() const {
   return layer;
}
