#include "engine.h"

/**
 * @brief Generates a unique ID for each object in the scene graph.
 *
 * Uses a static counter to produce a unique identifier for every instance of the Object class.
 *
 * @return unsigned int A unique ID.
 */
unsigned int ENG_API Eng::Object::generateUniqueID() {
   static unsigned int counter = 0;
   return counter++;
}

/**
 * @brief Default constructor for the Object class.
 *
 * Initializes the object with a unique ID.
 */
ENG_API Eng::Object::Object() : id{generateUniqueID()} {
}

/**
 * @brief Retrieves the unique ID of the object.
 *
 * Each object in the scene graph has a unique identifier assigned at creation.
 *
 * @return int The unique ID of the object.
 */
int ENG_API Eng::Object::getId() const {
   return id;
}

/**
 * @brief Retrieves the name of the current object.
 *
 * The name can be used to identify the object in the scene graph.
 *
 * @return std::string The name of the object.
 */
std::string ENG_API Eng::Object::getName() const {
   return name;
}

/**
 * @brief Sets the name of the object.
 *
 * Assigns a new name to the object for identification purposes.
 *
 * @param newName The new name of the object.
 */
void ENG_API Eng::Object::setName(std::string&& newName) {
   name = newName;
}
