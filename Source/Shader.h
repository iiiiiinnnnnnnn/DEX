#pragma once

#include "RenderContext.h"
#include "Model.h"

class Shader
{
public:
	Shader() {}
	virtual ~Shader() {}

	// •`‰æŠJn
	virtual void Begin(const RenderContext& rc) = 0;

	// ƒ‚ƒfƒ‹•`‰æ
	virtual void Draw(const RenderContext& rc, const Model* model) = 0;

	// •`‰æI—¹
	virtual void End(const RenderContext& rc) = 0;
};