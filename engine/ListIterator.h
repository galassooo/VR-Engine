#pragma once

class ENG_API ListIterator {
public:
	ListIterator(const std::vector<std::shared_ptr<Eng::ListElement>>& elements);
	~ListIterator() = default;
	void reset();
	bool hasNext() const;
	std::shared_ptr<Eng::ListElement> next();
private:
	std::vector<std::shared_ptr<Eng::ListElement>> elements;
	int currentIndex;
};