#include "engine.h"

/**
 * @brief Default constructor for the Node class.
 *
 * Initializes the node with no parent and an identity local transformation matrix.
 */
ENG_API Eng::Node::Node() : parent{nullptr}, localMatrix{glm::mat4{1.0f}} {
}

/**
 * @brief Sets the parent of the node.
 *
 * Updates the parent pointer of the current node to establish a relationship
 * in the scene graph hierarchy.
 *
 * @param p Pointer to the parent node.
 */
void ENG_API Eng::Node::setParent(Node *p) {
   parent = p;
}

/**
 * @brief Adds a child node to the list of current node's children.
 *
 * Establishes a parent-child relationship in the scene graph by adding the specified
 * child node to this node's list of children.
 *
 * @param child Shared pointer to the child node.
 */
void ENG_API Eng::Node::addChild(std::shared_ptr<Node> child) {
   children.push_back(child);
}

/**
 * @brief Retrieves the parent of the current node.
 *
 * @return Pointer to the parent node, or `nullptr` if the node has no parent.
 */
Eng::Node *Eng::Node::getParent() const {
   return parent;
}

/**
 * @brief Computes the final transformation matrix of the node.
 *
 * Combines the local transformation matrix of the current node with the
 * transformation matrices of all its parent nodes, recursively, to produce
 * the final matrix in world space.
 *
 * @return glm::mat4 The final transformation matrix in world space.
 */
glm::mat4 ENG_API Eng::Node::getFinalMatrix() const {
   if (parent) {
      return parent->getFinalMatrix() * localMatrix;
   }

   return localMatrix;
}

/**
 * @brief Sets the local transformation matrix of the current node.
 *
 * The local matrix defines the node's position, rotation, and scale relative to its parent.
 *
 * @param matrix The new local transformation matrix.
 */
void ENG_API Eng::Node::setLocalMatrix(const glm::mat4 &matrix) {
   localMatrix = matrix;
}

/**
 * @brief Retrieves the local transformation matrix of the current node.
 *
 * @return glm::mat4 The local transformation matrix.
 */
const glm::mat4 &Eng::Node::getLocalMatrix() const {
   return localMatrix;
}

/**
 * @brief Retrieves the list of children for the current node.
 *
 * @return std::vector<std::shared_ptr<Node>> pointer to a vector of shared pointers to the child nodes.
 */
std::vector<std::shared_ptr<Eng::Node> > *Eng::Node::getChildren() {
   return &children;
}
