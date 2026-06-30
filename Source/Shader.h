#pragma once

#include "Model.h"

class Scene;

class Shader
{
public:
	Shader() {}
	virtual ~Shader() {}

	// •`‰æŠJn
	virtual void Begin(Scene* rc) = 0;

	// ƒ‚ƒfƒ‹•`‰æ
	virtual void Draw(Scene* rc, const Model* model) = 0;

	// •`‰æI—¹
	virtual void End(Scene* rc) = 0;
};