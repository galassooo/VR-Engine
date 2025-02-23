#pragma once

/**
* @class Node
* @brief Represents a node in the scene graph with hierarchical transformations
*
* The Node class can have a parent and children, enabling hirarchical transformations within the scene graph
*/
class ENG_API Node : public Eng::Object {
public:
   Node();

   void setParent(Node *p);
   Node *getParent() const;

   void addChild(std::shared_ptr<Node> child);
   std::vector<std::shared_ptr<Node> > *getChildren();

   void setLocalMatrix(const glm::mat4 &matrix);
   const glm::mat4 &getLocalMatrix() const;

   glm::mat4 getFinalMatrix() const;

   virtual void render() override {}

protected:
   ///> pointer to parent node
   Node *parent;
   ///> vector of children
   std::vector<std::shared_ptr<Node> > children;
   ///> local matrix
   glm::mat4 localMatrix;
};
