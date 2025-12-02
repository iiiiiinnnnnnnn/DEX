#pragma once

#include "Camera.h"
#include "RenderState.h"

// 描画に必要な情報を構造体で定義する
struct RenderContext
{
	ID3D11DeviceContext* deviceContext;
	const RenderState* renderState;
	const Camera* camera;
};