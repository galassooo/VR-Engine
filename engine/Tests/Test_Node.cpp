#include "../engine.h"

/**
 * @brief Tests the hierarchy management of the Node class.
 */
void Eng::testNodeHierarchy() {
    // Create parent and child nodes
    Eng::Node parent, child1, child2;

    // Set parent-child relationships
    child1.setParent(&parent);
    child2.setParent(&parent);

    parent.addChild(std::make_shared<Eng::Node>(child1));
    parent.addChild(std::make_shared<Eng::Node>(child2));

    // Validate parent-child relationships
    assert(child1.getParent() == &parent);
    assert(child2.getParent() == &parent);

    auto children = parent.getChildren();
    assert(children->size() == 2);
    assert((*children)[0].get() == &child1);
    assert((*children)[1].get() == &child2);

    std::cout << "Node Hierarchy Test Passed!" << std::endl;
}

/**
 * @brief Tests the transformation matrix functionality of the Node class.
 */
void Eng::testNodeTransformations() {
    // Create a node
    Eng::Node node;

    // Set a local transformation matrix
    glm::mat4 localMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 2.0f, 3.0f));
    node.setLocalMatrix(localMatrix);

    // Validate local matrix
    assert(node.getLocalMatrix() == localMatrix);

    // Assume no parent, so final matrix should equal local matrix
    glm::mat4 finalMatrix = node.getFinalMatrix();
    assert(finalMatrix == localMatrix);

    std::cout << "Node Transformations Test Passed!" << std::endl;
}