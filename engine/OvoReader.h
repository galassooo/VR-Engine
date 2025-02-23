//
// Created by Martina ♛ on 06/12/24.

#pragma once


/**
 * @class OvoReader
 * @brief A class responsible for parsing and loading 3D scene data from OVO format files.
 *
 * The OvoReader class provides functionality to read and parse OVO format files,
 * constructing a scene graph with meshes, materials, lights, and other 3D scene elements.
 * It maintains internal state about the scene hierarchy and manages the relationships
 * between different scene elements during the parsing process.
 */
class ENG_API OvoReader final {
public:

   std::shared_ptr<Eng::Node> parseOvoFile(const std::string &filename);
   void printGraph() const;


private:

   /**
   * @class NodeInfo
   * @brief Helper class for managing node hierarchy during scene graph construction.
   *
   * Maintains information about a node's remaining children count during the parsing process.
   */
   class ENG_API NodeInfo final {
   public:
      NodeInfo(const unsigned int children, const std::shared_ptr<Eng::Node> &node)
    : remainingChildren(children), node(node) {}

      int remainingChildren; ///< Number of children yet to be processed
      std::shared_ptr<Eng::Node> node; ///< The node being tracked
   };

   static void parseBone(unsigned int &position, const char *data);
   static glm::vec3  decompressNormal(unsigned int packedNormal);
   static glm::vec2 decompressTexCoords(unsigned int packedTexCoords);
   static std::shared_ptr<Eng::Node> parseLight(unsigned int &position, const char *data);
   static void analyzeObject(unsigned int &position, char *data);
   static std::shared_ptr<Eng::Node> analyzeNode(unsigned int &position, const char *data, unsigned int &children);
   static void printGraphHelper(const std::shared_ptr<Eng::Node> &node, int depth);

   void manageSceneGraph(std::shared_ptr<Eng::Node> &node, unsigned int children);
   void parseMaterial(unsigned int &position, const char *data);
   std::shared_ptr<Eng::Node> parseMesh(unsigned int &position, unsigned int chunkId, char *data, unsigned int &children);

   ///< Stack for managing node hierarchy during parsing
   std::stack<NodeInfo> nodeStack;
   ///< Root node of the scene graph
   std::shared_ptr<Eng::Node> root;
   ///< Map of material names to material objects
   std::unordered_map<std::string, std::shared_ptr<Eng::Material>> materials;
   ///< directory of the file
   std::string basePath;
};

