#pragma once

/*
* @class Object
* @brief Base Class for all objects in the scene graph.
* 
* Root class for all drawable entities in the engine.
* Each object is assigned a unique ID and has a virtual render() function
*/
class ENG_API Object {
public:
	Object();

	virtual ~Object() = default;

	virtual void render() = 0;
	int getId() const;
	std::string getName() const;
	void setName(std::string&& newName);

protected:
	///> object unique identifier
	unsigned int id;
	///> object name
	std::string name;

private:
	static unsigned int generateUniqueID();
};