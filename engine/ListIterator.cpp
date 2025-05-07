#include "engine.h"

Eng::ListIterator::ListIterator(const std::vector<std::shared_ptr<Eng::ListElement>>& elements)
	: elements(elements), currentIndex(0) {
}

void Eng::ListIterator::reset() {
	currentIndex = 0;
}

bool Eng::ListIterator::hasNext() const {
	return currentIndex < elements.size();
}

std::shared_ptr<Eng::ListElement> Eng::ListIterator::next() {
	if (!hasNext()) {
		return nullptr;
	}
	return elements[currentIndex++];
}