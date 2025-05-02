#pragma once

class ENG_API RenderPass {
public:
	virtual void render(Eng::ListElement element);
	void setBlendingMode(bool mode) { blendingMode = mode; }
protected:
	bool blendingMode = false;
};