#pragma once

class ENG_API MultipassRenderer {
public:
	static MultipassRenderer& getInstance();

	void render(Eng::List& renderList);
private:
	MultipassRenderer() = default;
	~MultipassRenderer() = default;
};
