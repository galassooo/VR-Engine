#pragma once

struct RenderPassContext {
	RenderPassContext(Eng::CullingMode cullingMode, Eng::RenderLayer renderLayer) :
		culling{ cullingMode }, renderLayer{ renderLayer }
	{
	}
	~RenderPassContext() = default;

	Eng::CullingMode culling;
	Eng::RenderLayer renderLayer;
};
