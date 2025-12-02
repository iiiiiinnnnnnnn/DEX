#pragma once

#include <DirectXMath.h>

struct DirectionalLight
{
	DirectX::XMFLOAT3 direction;
	DirectX::XMFLOAT3 color;
};

class LightManager
{
public:
	// ディレクショナルライト設定
	void SetDirectionalLight(DirectionalLight& light) { directionalLight = light; }

	// ディレクショナルライト取得
	const DirectionalLight& GetDirectionalLight() const { return directionalLight; }

private:
	DirectionalLight directionalLight;
};