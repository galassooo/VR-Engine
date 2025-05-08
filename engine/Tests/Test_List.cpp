#include "../Engine.h"

/**
 * @brief Tests the node addition and clearing functionality of the List class.
 */
void Eng::testListNodeManagement() {
    // Create a List instance
    Eng::List list;

    // Mock nodes and transformation matrices
    auto node1 = std::make_shared<Eng::Node>();
    auto node2 = std::make_shared<Eng::Node>();
    glm::mat4 transform1 = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 2.0f, 3.0f));
    glm::mat4 transform2 = glm::translate(glm::mat4(1.0f), glm::vec3(4.0f, 5.0f, 6.0f));

    // Add nodes to the list
    list.addNode(node1, transform1);
    list.addNode(node2, transform2);

    // Render the list with a mock view matrix
    glm::mat4 viewMatrix = glm::mat4(1.0f);
    list.setEyeViewMatrix(viewMatrix);
    list.render();

    // Clear the list
    list.clear();

    std::cout << "List Node Management Test Passed!" << std::endl;
}

/**
 * @brief Tests the world coordinates and node retrieval in ListElement.
 */
void Eng::testListElement() {
    // Mock a node and transformation matrix
    auto node = std::make_shared<Eng::Node>();
    glm::mat4 worldTransform = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 2.0f, 3.0f));

    // Create a ListElement
    Eng::ListElement element(node, worldTransform);

    // Test getNode
    assert(element.getNode() == node);

    // Test getWorldCoordinates
    assert(element.getWorldCoordinates() == worldTransform);

    std::cout << "ListElement Test Passed!" << std::endl;
}

/**
 * @brief Tests the ordering of lights and nodes in the List class.
 */
void Eng::testListOrdering() {
    // Create a List instance
    Eng::List list;

    // Mock lights and nodes
    auto directionalLight = std::make_shared<Eng::DirectionalLight>(glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
    auto pointLight = std::make_shared<Eng::PointLight>(glm::vec3(1.0f, 0.5f, 0.5f), 0.5f);
    auto node1 = std::make_shared<Eng::Node>();
    auto node2 = std::make_shared<Eng::Node>();

    glm::mat4 transform1 = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 2.0f, 3.0f));
    glm::mat4 transform2 = glm::translate(glm::mat4(1.0f), glm::vec3(4.0f, 5.0f, 6.0f));

    // Add lights first
    list.addNode(directionalLight, transform1);
    list.addNode(pointLight, transform2);

    // Add nodes after lights
    list.addNode(node1, transform1);
    list.addNode(node2, transform2);

    // Validate ordering
    auto elements = list.getElements();
    assert(dynamic_cast<Eng::Light*>(elements[0]->getNode().get()) != nullptr); // First element is a Light
    assert(dynamic_cast<Eng::Light*>(elements[1]->getNode().get()) != nullptr); // Second element is a Light
    assert(dynamic_cast<Eng::Node*>(elements[2]->getNode().get()) != nullptr);  // Third element is a Node
    assert(dynamic_cast<Eng::Node*>(elements[3]->getNode().get()) != nullptr);  // Fourth element is a Node

    std::cout << "List Ordering Test Passed!" << std::endl;
}