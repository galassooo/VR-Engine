//
// Created by Martina ♛ on 06/12/24.
//

#include "engine.h"
#include <glm/gtc/packing.hpp>


class ENG_API OvObject final {
public:
   enum class ENG_API Type : int {
      OBJECT = 0, NODE, OBJECT2D, OBJECT3D, LIST,

      BUFFER, SHADER, TEXTURE, FILTER, MATERIAL, FBO, QUAD,
      BOX, SKYBOX, FONT, CAMERA, LIGHT, BONE, MESH, SKINNED, INSTANCED,
      PIPELINE, EMITTER, ANIM, PHYSICS, LAST,
   };
};

// Stripped-down redefinition of OvMesh (just for the subtypes):
class ENG_API OvMesh final {
public:
   enum class ENG_API Subtype : int {
      DEFAULT = 0, NORMALMAPPED, TESSELLATED, LAST,
   };
};

class ENG_API OvLight final {
public:
   enum class Subtype : int {
      OMNI = 0, DIRECTIONAL, SPOT, LAST,
   };
};

/**
 * @brief Parses an OVO file and constructs the scene graph.
 * @param filename The path to the OVO file to be parsed.
 * @return A shared pointer to the root node of the constructed scene graph.
 */
std::shared_ptr<Eng::Node> Eng::OvoReader::parseOvoFile(const std::string &filename) {
   using namespace std;

   FILE *dat = fopen(filename.c_str(), "rb");
   if (dat == nullptr) {
      cout << "ERROR: unable to open file '" << filename << "'" << endl;
      return nullptr;
   }

   size_t lastSlash = filename.find_last_of("/\\");
   if (lastSlash != std::string::npos) {
      basePath = filename.substr(0, lastSlash + 1);
   } else {
      basePath = "./";
   }

   // Configure stream:
   cout.precision(2);
   cout << fixed;

   /////////////////
   // Parse chuncks:
   unsigned int chunkId, chunkSize;
   while (true) {
      fread(&chunkId, sizeof(unsigned int), 1, dat);
      if (feof(dat))
         break;
      fread(&chunkSize, sizeof(unsigned int), 1, dat);

      cout << "\n[chunk id: " << chunkId << ", chunk size: " << chunkSize << ", chunk type: ";

      // Load whole chunk into memory:
      char *data = new char[chunkSize];
      if (fread(data, sizeof(char), chunkSize, dat) != chunkSize) {
         cout << "ERROR: unable to read from file '" << filename << "'" << endl;
         fclose(dat);
         delete[] data;
         return nullptr;
      }

      // Parse chunk information according to its type:
      unsigned int position = 0;

      shared_ptr<Node> node = nullptr;
      unsigned int children = 0;

      switch (static_cast<OvObject::Type>(chunkId)) {
         ///////////////////////////////
         case OvObject::Type::OBJECT: //
         {
            analyzeObject(position, data);
         }
         break;


         /////////////////////////////
         case OvObject::Type::NODE: //
         {
            node = analyzeNode(position, data, children);
         }
         break;


         /////////////////////////////////
         case OvObject::Type::MATERIAL: //
         {
            parseMaterial(position, data);
         }
         break;


         ////////////////////////////////
         case OvObject::Type::MESH: //
         case OvObject::Type::SKINNED: {
            node = parseMesh(position, chunkId, data, children);
         }
         break;


         //////////////////////////////
         case OvObject::Type::LIGHT: //
         {
            node = parseLight(position, data);
         }
         break;


         /////////////////////////////
         case OvObject::Type::BONE: //
         {
            parseBone(position, data);
         }
         break;


         ///////////
         default: //
            cout << "UNKNOWN]" << endl;
            cout << "ERROR: corrupted or bad data in file " << filename << endl;
            fclose(dat);
            delete[] data;
            return nullptr;
      }

      if (node != nullptr) {
         manageSceneGraph(node, children);
      }
      // Release chunk memory:
      delete[] data;
   }

   fclose(dat);
   cout << "\nFile parsed" << endl;

   return root;
}

/**
 * @brief Manages scene graph construction by handling parent-child relationships.
 * @param node The node to be added to the scene graph.
 * @param children Number of children expected for this node.
 */
void ENG_API Eng::OvoReader::manageSceneGraph(std::shared_ptr<Node>& node, const unsigned int children) {
   // add node to parent and parent to node
   if(node.get()->getName() == "[root]") {
      root = node;
   }
   if (!nodeStack.empty()) {
      NodeInfo &currentParent = nodeStack.top();
      currentParent.node->addChild(node);
      node->setParent(currentParent.node.get());
      currentParent.remainingChildren--;

      // if parent has no children left, then remove it from the stack
      if (currentParent.remainingChildren == 0) {
         nodeStack.pop();
      }
   }

   // if current node has children, add it to the stack
   if (children > 0) {
      nodeStack.emplace(children, node);
   }
}

/**
 * @brief Parses bone data from the OVO file.
 * @param position Current position in the data buffer.
 * @param data Pointer to the data buffer containing bone information.
 */
void ENG_API Eng::OvoReader::parseBone(unsigned int &position, const char *data) {
   using namespace std;
   cout << "bone]" << endl;

   // Bone name:
   char boneName[FILENAME_MAX];
   strcpy(boneName, data + position);
   cout << "   Name  . . . . :  " << boneName << endl;
   position += (unsigned int) strlen(boneName) + 1;

   // Bone matrix:
   glm::mat4 matrix;
   memcpy(&matrix, data + position, sizeof(glm::mat4));
   position += sizeof(glm::mat4);

   // Nr. of children nodes:
   unsigned int children;
   memcpy(&children, data + position, sizeof(unsigned int));
   cout << "   Nr. children  :  " << children << endl;
   position += sizeof(unsigned int);

   // Optional target node, or [none] if not used:
   char targetName[FILENAME_MAX];
   strcpy(targetName, data + position);
   cout << "   Target node . :  " << targetName << endl;
   position += static_cast<unsigned int>(strlen(targetName)) + 1;

   // Mesh bounding box minimum corner:
   glm::vec3 bBoxMin;
   memcpy(&bBoxMin, data + position, sizeof(glm::vec3));
   cout << "   BBox minimum  :  " << bBoxMin.x << ", " << bBoxMin.y << ", " << bBoxMin.z << endl;
   position += sizeof(glm::vec3);

   // Mesh bounding box maximum corner:
   glm::vec3 bBoxMax;
   memcpy(&bBoxMax, data + position, sizeof(glm::vec3));
   cout << "   BBox maximum  :  " << bBoxMax.x << ", " << bBoxMax.y << ", " << bBoxMax.z << endl;
   position += sizeof(glm::vec3);
}

/**
 * @brief Parses light data from the OVO file.
 * @param position Current position in the data buffer.
 * @param data Pointer to the data buffer containing light information.
 * @return Shared pointer to the created light node.
 */
std::shared_ptr<Eng::Node> ENG_API Eng::OvoReader::parseLight(unsigned int &position, const char *data) {
   using namespace std;
   cout << "light]" << endl;

   // Light name:
   char lightName[FILENAME_MAX];
   strcpy(lightName, data + position);
   cout << "   Name  . . . . :  " << lightName << endl;
   position += static_cast<unsigned int>(strlen(lightName)) + 1;

   // Light matrix:
   glm::mat4 matrix;
   memcpy(&matrix, data + position, sizeof(glm::mat4));

   cout << "MATRIX: " << glm::to_string(matrix) << endl;

   position += sizeof(glm::mat4);

   // Nr. of children nodes:
   unsigned int children;
   memcpy(&children, data + position, sizeof(unsigned int));
   cout << "   Nr. children  :  " << children << endl;
   position += sizeof(unsigned int);

   // Optional target node name, or [none] if not used:
   char targetName[FILENAME_MAX];
   strcpy(targetName, data + position);
   cout << "   Target node . :  " << targetName << endl;
   position += static_cast<unsigned int>(strlen(targetName)) + 1;

   // Light subtype (see OvLight SUBTYPE enum):
   unsigned char subtype;
   memcpy(&subtype, data + position, sizeof(unsigned char));
   char subtypeName[FILENAME_MAX];
   switch (static_cast<OvLight::Subtype>(subtype)) {
      case OvLight::Subtype::DIRECTIONAL: strcpy(subtypeName, "directional");
         break;
      case OvLight::Subtype::OMNI: strcpy(subtypeName, "omni");
         break;
      case OvLight::Subtype::SPOT: strcpy(subtypeName, "spot");
         break;
      default: strcpy(subtypeName, "UNDEFINED");
   }
   cout << "   Subtype . . . :  " << static_cast<int>(subtype) << " (" << subtypeName << ")" << endl;
   position += sizeof(unsigned char);

   // Light color:
   glm::vec3 color;
   memcpy(&color, data + position, sizeof(glm::vec3));
   cout << "   Color . . . . :  " << color.r << ", " << color.g << ", " << color.b << endl;
   position += sizeof(glm::vec3);

   // Influence radius:
   float radius;
   memcpy(&radius, data + position, sizeof(float));
   cout << "   Radius  . . . :  " << radius << endl;
   position += sizeof(float);

   // Direction:
   glm::vec3 direction;
   memcpy(&direction, data + position, sizeof(glm::vec3));
   cout << "   Direction . . :  " << direction.r << ", " << direction.g << ", " << direction.b << endl;
   position += sizeof(glm::vec3);

   // Cutoff:
   float cutoff;
   memcpy(&cutoff, data + position, sizeof(float));
   cout << "   Cutoff  . . . :  " << cutoff << endl;
   position += sizeof(float);

   // Exponent:
   float spotExponent;
   memcpy(&spotExponent, data + position, sizeof(float));
   cout << "   Spot exponent :  " << spotExponent << endl;
   position += sizeof(float);

   // Cast shadow flag:
   unsigned char castShadows;
   memcpy(&castShadows, data + position, sizeof(unsigned char));
   cout << "   Cast shadows  :  " << static_cast<int>(castShadows) << endl;
   position += sizeof(unsigned char);

   // Volumetric lighting flag:
   unsigned char isVolumetric;
   memcpy(&isVolumetric, data + position, sizeof(unsigned char));
   cout << "   Volumetric  . :  " << static_cast<int>(isVolumetric) << endl;
   position += sizeof(unsigned char);


   std::shared_ptr<Eng::Node> light;
   switch (static_cast<OvLight::Subtype>(subtype)) {
      case OvLight::Subtype::DIRECTIONAL: {
         auto dirLight = std::make_shared<Eng::DirectionalLight>(color, direction);
         dirLight->setName(lightName);
         dirLight->setLocalMatrix(matrix);

         light = std::static_pointer_cast<Eng::Node>(dirLight);
         break;
      }
      case OvLight::Subtype::OMNI: {
         auto pointLight = std::make_shared<Eng::PointLight>(color, radius);
         pointLight->setName(lightName);
         pointLight->setLocalMatrix(matrix);
         light = std::static_pointer_cast<Eng::Node>(pointLight);
         break;
      }
      case OvLight::Subtype::SPOT: {
         auto spotLight = std::make_shared<Eng::SpotLight>(
            color, direction, cutoff, spotExponent, radius);
         spotLight->setName(lightName);
         spotLight->setLocalMatrix(matrix);
         light = std::static_pointer_cast<Eng::Node>(spotLight);
         break;
      }
      default: ;
   }
   return light;
}

/**
  * @brief Analyzes object header information from the OVO file.
  * @param position Current position in the data buffer.
  * @param data Pointer to the data buffer containing object information.
  */
void ENG_API Eng::OvoReader::analyzeObject(unsigned int &position, char *data) {
   using namespace std;
   cout << "version]" << endl;

   // OVO revision number:
   unsigned int versionId;
   memcpy(&versionId, data + position, sizeof(unsigned int));
   cout << "   Version . . . :  " << versionId << endl;
   position += sizeof(unsigned int);
}

/**
     * @brief Analyzes node data from the OVO file.
     * @param position Current position in the data buffer.
     * @param data Pointer to the data buffer containing node information.
     * @param children Reference to store the number of child nodes.
     * @return Shared pointer to the created node.
     */
std::shared_ptr<Eng::Node> ENG_API Eng::OvoReader::analyzeNode(unsigned int &position, const char *data,
                                                               unsigned int &children) {
   using namespace std;
   cout << "node]" << endl;

   const auto newNode = std::make_shared<Node>();

   // Node name:
   char nodeName[FILENAME_MAX];
   strcpy(nodeName, data + position);
   cout << "   Name  . . . . :  " << nodeName << endl;
   position += static_cast<unsigned int>(strlen(nodeName)) + 1;

   newNode->setName(nodeName);

   // Node matrix:
   glm::mat4 matrix;
   memcpy(&matrix, data + position, sizeof(glm::mat4));
   position += sizeof(glm::mat4);

   newNode->setLocalMatrix(matrix);

   memcpy(&children, data + position, sizeof(unsigned int));
   cout << "   Nr. children :  " << children << endl;
   position += sizeof(unsigned int);

   // Optional target node, [none] if not used:
   char targetName[FILENAME_MAX];
   strcpy(targetName, data + position);
   cout << "   Target node . :  " << targetName << endl;
   position += static_cast<unsigned int>(strlen(targetName)) + 1;

   return newNode;
}

/**
 * @brief Parses material data from the OVO file.
 * @param position Current position in the data buffer.
 * @param data Pointer to the data buffer containing material information.
 */

void ENG_API Eng::OvoReader::parseMaterial(unsigned int &position, const char *data) {
   using namespace std;
   cout << "material]" << endl;

   // Material name:
   char materialName[FILENAME_MAX];
   strcpy(materialName, data + position);
   cout << "   Name  . . . . :  " << materialName << endl;
   position += static_cast<unsigned int>(strlen(materialName)) + 1;

   // Material term colors, starting with emissive:
   glm::vec3 emission, albedo;
   memcpy(&emission, data + position, sizeof(glm::vec3));
   cout << "   Emission  . . :  " << emission.r << ", " << emission.g << ", " << emission.b << endl;
   position += sizeof(glm::vec3);

   // Albedo:
   memcpy(&albedo, data + position, sizeof(glm::vec3));
   cout << "   Albedo  . . . :  " << albedo.r << ", " << albedo.g << ", " << albedo.b << endl;
   position += sizeof(glm::vec3);

   // Roughness factor:
   float roughness;
   memcpy(&roughness, data + position, sizeof(float));
   cout << "   Roughness . . :  " << roughness << endl;
   position += sizeof(float);

   // Metalness factor:
   float metalness;
   memcpy(&metalness, data + position, sizeof(float));
   cout << "   Metalness . . :  " << metalness << endl;
   position += sizeof(float);

   // Transparency factor:
   float alpha;
   memcpy(&alpha, data + position, sizeof(float));
   cout << "   Transparency  :  " << alpha << endl;
   position += sizeof(float);

   // Albedo texture filename, or [none] if not used:
   char textureName[FILENAME_MAX];
   strcpy(textureName, data + position);
   cout << "   Albedo tex. . :  " << textureName << endl;
   position += static_cast<unsigned int>(strlen(textureName)) + 1;

   // add material
   const auto material = std::make_shared<Eng::Material>(albedo, alpha, roughness);

   if (strcmp(textureName, "[none]") != 0) {
      std::string texturePath = basePath + std::string(textureName);
      const auto texture = std::make_shared<Eng::Texture>(texturePath);
      material->setDiffuseTexture(texture);
   }
   materials[materialName] = material;


   // Normal map filename, or [none] if not used:
   char normalMapName[FILENAME_MAX];
   std::strcpy(normalMapName, data + position);
   cout << "   Normalmap tex.:  " << normalMapName << endl;
   position += static_cast<unsigned int>(strlen(normalMapName)) + 1;

   // Height map filename, or [none] if not used:
   char heightMapName[FILENAME_MAX];
   strcpy(heightMapName, data + position);
   cout << "   Heightmap tex.:  " << heightMapName << endl;
   position += static_cast<unsigned int>(strlen(heightMapName)) + 1;

   // Roughness map filename, or [none] if not used:
   char roughnessMapName[FILENAME_MAX];
   strcpy(roughnessMapName, data + position);
   cout << "   Roughness tex.:  " << roughnessMapName << endl;
   position += static_cast<unsigned int>(strlen(roughnessMapName)) + 1;

   // Metalness map filename, or [none] if not used:
   char metalnessMapName[FILENAME_MAX];
   strcpy(metalnessMapName, data + position);
   cout << "   Metalness tex.:  " << metalnessMapName << endl;
   position += static_cast<unsigned int>(strlen(metalnessMapName)) + 1;
}

/**
  * @brief Parses mesh data from the OVO file.
  * @param position Current position in the data buffer.
  * @param chunkId ID of the chunk being parsed.
  * @param data Pointer to the data buffer containing mesh information.
  * @param children Reference to store the number of child nodes.
  * @return Shared pointer to the created mesh node.
  */
std::shared_ptr<Eng::Node> ENG_API Eng::OvoReader::parseMesh(
    unsigned int& position, unsigned int chunkId, char* data, unsigned int& children)
{
    using namespace std;

    // Parse mesh name.
    char meshName[FILENAME_MAX];
    strcpy(meshName, data + position);
    position += static_cast<unsigned int>(strlen(meshName)) + 1;
    cout << "   Name  . . . . :  " << meshName << endl;

    // Parse transformation matrix.
    glm::mat4 matrix;
    memcpy(&matrix, data + position, sizeof(glm::mat4));
    position += sizeof(glm::mat4);
    cout << "MATRIX: " << glm::to_string(matrix) << endl;

    // Parse number of children.
    memcpy(&children, data + position, sizeof(unsigned int));
    cout << "   Nr. children:  " << children << endl;
    position += sizeof(unsigned int);

    // Parse target node.
    char targetName[FILENAME_MAX];
    strcpy(targetName, data + position);
    cout << "   Target node . :  " << targetName << endl;
    position += static_cast<unsigned int>(strlen(targetName)) + 1;

    // Parse mesh subtype.
    unsigned char subtype;
    memcpy(&subtype, data + position, sizeof(unsigned char));
    position += sizeof(unsigned char);
    char subtypeName[FILENAME_MAX];
    switch (static_cast<OvMesh::Subtype>(subtype)) {
    case OvMesh::Subtype::DEFAULT: strcpy(subtypeName, "standard"); break;
    case OvMesh::Subtype::NORMALMAPPED: strcpy(subtypeName, "normal-mapped"); break;
    case OvMesh::Subtype::TESSELLATED: strcpy(subtypeName, "tessellated"); break;
    default: strcpy(subtypeName, "UNDEFINED");
    }
    cout << "   Subtype . . . :  " << (int)subtype << " (" << subtypeName << ")" << endl;

    // Parse material name.
    char materialName[FILENAME_MAX];
    strcpy(materialName, data + position);
    cout << "   Material  . . :  " << materialName << endl;
    position += static_cast<unsigned int>(strlen(materialName)) + 1;

    // Parse bounding sphere radius.
    float radius;
    memcpy(&radius, data + position, sizeof(float));
    cout << "   Radius  . . . :  " << radius << endl;
    position += sizeof(float);

    // Parse bounding box.
    glm::vec3 bBoxMin, bBoxMax;
    memcpy(&bBoxMin, data + position, sizeof(glm::vec3));
    position += sizeof(glm::vec3);
    cout << "   BBox minimum  :  " << bBoxMin.x << ", " << bBoxMin.y << ", " << bBoxMin.z << endl;
    memcpy(&bBoxMax, data + position, sizeof(glm::vec3));
    position += sizeof(glm::vec3);
    cout << "   BBox maximum  :  " << bBoxMax.x << ", " << bBoxMax.y << ", " << bBoxMax.z << endl;

    // Parse physics properties if present.
    unsigned char hasPhysics;
    memcpy(&hasPhysics, data + position, sizeof(unsigned char));
    position += sizeof(unsigned char);
    cout << "   Physics . . . :  " << (int)hasPhysics << endl;
    if (hasPhysics) {
        struct PhysProps {
            unsigned char type;
            unsigned char contCollisionDetection;
            unsigned char collideWithRBodies;
            unsigned char hullType;
            glm::vec3 massCenter;
            float mass;
            float staticFriction;
            float dynamicFriction;
            float bounciness;
            float linearDamping;
            float angularDamping;
            unsigned int nrOfHulls;
            unsigned int _pad;
            void* physObj;
            void* hull;
        } mp;
        memcpy(&mp, data + position, sizeof(PhysProps));
        position += sizeof(PhysProps);
        cout << "      Type . . . :  " << (int)mp.type << endl;
        cout << "      Hull type  :  " << (int)mp.hullType << endl;
        cout << "      Cont. coll.:  " << (int)mp.contCollisionDetection << endl;
        cout << "      Col. bodies:  " << (int)mp.collideWithRBodies << endl;
        cout << "      Center . . :  " << mp.massCenter.x << ", " << mp.massCenter.y << ", " << mp.massCenter.z << endl;
        cout << "      Mass . . . :  " << mp.mass << endl;
        cout << "      Static . . :  " << mp.staticFriction << endl;
        cout << "      Dynamic  . :  " << mp.dynamicFriction << endl;
        cout << "      Bounciness :  " << mp.bounciness << endl;
        cout << "      Linear . . :  " << mp.linearDamping << endl;
        cout << "      Angular  . :  " << mp.angularDamping << endl;
        cout << "      Nr. hulls  :  " << mp.nrOfHulls << endl;
        if (mp.nrOfHulls) {
            for (unsigned int c = 0; c < mp.nrOfHulls; c++) {
                unsigned int nrOfVertices;
                memcpy(&nrOfVertices, data + position, sizeof(unsigned int));
                position += sizeof(unsigned int);
                unsigned int nrOfFaces;
                memcpy(&nrOfFaces, data + position, sizeof(unsigned int));
                position += sizeof(unsigned int);
                glm::vec3 centroid;
                memcpy(&centroid, data + position, sizeof(glm::vec3));
                position += sizeof(glm::vec3);
                for (unsigned int v = 0; v < nrOfVertices; v++) {
                    glm::vec3 vertex;
                    memcpy(&vertex, data + position, sizeof(glm::vec3));
                    position += sizeof(glm::vec3);
                }
                for (unsigned int f = 0; f < nrOfFaces; f++) {
                    unsigned int face[3];
                    memcpy(face, data + position, sizeof(unsigned int) * 3);
                    position += sizeof(unsigned int) * 3;
                }
            }
        }
    }

    // Parse LOD count.
    unsigned int LODs;
    memcpy(&LODs, data + position, sizeof(unsigned int));
    position += sizeof(unsigned int);
    cout << "   Nr. of LODs   :  " << LODs << endl;

    vector<unsigned int> verticesPerLOD(LODs);
    std::vector<Eng::Vertex> finalVerts;
    std::vector<unsigned int> finalIndices;
    for (unsigned int l = 0; l < LODs; l++) {
        cout << "   Current LOD . :  " << l + 1 << "/" << LODs << endl;
        unsigned int vertexCount, faceCount;
        memcpy(&vertexCount, data + position, sizeof(unsigned int));
        position += sizeof(unsigned int);
        cout << "   Nr. vertices  :  " << vertexCount << endl;
        verticesPerLOD[l] = vertexCount;
        memcpy(&faceCount, data + position, sizeof(unsigned int));
        position += sizeof(unsigned int);
        cout << "   Nr. faces . . :  " << faceCount << endl;

        std::vector<Eng::Vertex> verts;
        for (unsigned int c = 0; c < vertexCount; c++) {
            Eng::Vertex newVertex;
            glm::vec3 pos;
            memcpy(&pos, data + position, sizeof(glm::vec3));
            position += sizeof(glm::vec3);
            newVertex.setPosition(pos);
            unsigned int normalData;
            memcpy(&normalData, data + position, sizeof(unsigned int));
            position += sizeof(unsigned int);
            newVertex.setNormal(decompressNormal(normalData));
            unsigned int textureData;
            memcpy(&textureData, data + position, sizeof(unsigned int));
            position += sizeof(unsigned int);
            newVertex.setTexCoords(decompressTexCoords(textureData));
            unsigned int tangentData;
            memcpy(&tangentData, data + position, sizeof(unsigned int));
            position += sizeof(unsigned int);
            if (l == 0) {
                verts.push_back(newVertex);
            }
        }

        std::vector<unsigned int> meshIndices;
        if (l == 0) {
            meshIndices.reserve(faceCount * 3);
        }
        for (unsigned int c = 0; c < faceCount; c++) {
            unsigned int face[3];
            memcpy(face, data + position, sizeof(unsigned int) * 3);
            position += sizeof(unsigned int) * 3;
            if (l == 0) {
                meshIndices.push_back(face[0]);
                meshIndices.push_back(face[1]);
                meshIndices.push_back(face[2]);
            }
        }
        if (l == 0) {
            // Store LOD 0 data.
            // (finalVerts and finalIndices hold the geometry we want to use)
            finalVerts = verts;
            finalIndices = meshIndices;
        }
    }

    if (chunkId == static_cast<unsigned int>(OvObject::Type::SKINNED)) {
        glm::mat4 poseMatrix;
        memcpy(&poseMatrix, data + position, sizeof(glm::mat4));
        position += sizeof(glm::vec4);
        unsigned int nrOfBones;
        memcpy(&nrOfBones, data + position, sizeof(unsigned int));
        position += sizeof(unsigned int);
        cout << "   Nr. bones . . :  " << nrOfBones << endl;
        for (unsigned int c = 0; c < nrOfBones; c++) {
            char boneName[FILENAME_MAX];
            strcpy(boneName, data + position);
            position += static_cast<unsigned int>(strlen(boneName)) + 1;
            cout << "      Bone name  :  " << boneName << " (" << c << ")" << endl;
            glm::mat4 boneMatrix;
            memcpy(&boneMatrix, data + position, sizeof(glm::mat4));
            position += sizeof(glm::mat4);
        }
        for (unsigned int l = 0; l < LODs; l++) {
            cout << "   Current LOD . :  " << l + 1 << "/" << LODs << endl;
            for (unsigned int c = 0; c < verticesPerLOD[l]; c++) {
                unsigned int boneIndex[4];
                memcpy(boneIndex, data + position, sizeof(unsigned int) * 4);
                position += sizeof(unsigned int) * 4;
                unsigned short boneWeightData[4];
                memcpy(boneWeightData, data + position, sizeof(unsigned short) * 4);
                position += sizeof(unsigned short) * 4;
            }
        }
    }

    // Use Builder chaining to create the final mesh.
    auto& builder = Eng::Builder::getInstance();
    std::shared_ptr<Eng::Mesh> finalMesh =
        builder.setName(std::move(std::string(meshName)))
        .setLocalMatrix(matrix)
        .addVertices(finalVerts)
        .addIndices(finalIndices)
        .setMaterial(materials.find(materialName) != materials.end() ? materials.at(materialName) : nullptr)
        .build();

    return std::static_pointer_cast<Eng::Node>(finalMesh);
}

/**
 * @brief Decompresses a packed normal vector.
 * @param packedNormal Compressed normal data.
 * @return Decompressed normal vector.
 */

glm::vec3 ENG_API Eng::OvoReader::decompressNormal(const unsigned int packedNormal) {
   glm::vec4 normal = glm::unpackSnorm3x10_1x2(packedNormal);
   return {normal.x, normal.y, normal.z};
}

/**
 * @brief Decompresses packed texture coordinates.
 * @param packedTexCoords Compressed texture coordinate data.
 * @return Decompressed texture coordinates.
 */
glm::vec2 ENG_API Eng::OvoReader::decompressTexCoords(const unsigned int packedTexCoords) {
   return glm::unpackHalf2x16(packedTexCoords);
}

/**
  * @brief Prints the current scene graph structure to standard output.
  */
void Eng::OvoReader::printGraph() const {
   printGraphHelper(root, 0);
}

/**
   * @brief Helper function for recursive scene graph printing.
   * @param node Current node being printed.
   * @param depth Current depth in the scene graph hierarchy.
   */
void Eng::OvoReader::printGraphHelper(const std::shared_ptr<Eng::Node> &node, const int depth) {
   if (!node) return;
   const std::string indent(depth * 2, ' ');

   std::cout << indent << "+ " << node->getName() << std::endl;
   for (const auto &children = node->getChildren(); auto &child: *children) {
      printGraphHelper(child, depth + 1);
   }
}
