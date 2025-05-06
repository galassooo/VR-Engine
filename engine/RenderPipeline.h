#pragma once

class ENG_API RenderPipeline {
public:
	bool init();
	void setRenderList(std::shared_ptr<Eng::List> renderList);
	void run();
private:
	std::shared_ptr<Eng::List> renderList;
};
