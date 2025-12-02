#pragma once

#include <wrl.h>
#include <d3d11.h>

// サンプラステート
enum class SamplerState
{
	PointWrap,
	PointClamp,
	LinearWrap,
	LinearClamp,
	EnumCount
};

// レンダーステート
class RenderState
{
public:
	RenderState(ID3D11Device* device);
	~RenderState() = default;

	// サンプラステート取得
	ID3D11SamplerState* GetSamplerState(SamplerState state) const
	{
		return samplerStates[static_cast<int>(state)].Get();
	}

private:
	Microsoft::WRL::ComPtr<ID3D11SamplerState>
		samplerStates[static_cast<int>(SamplerState::EnumCount)];
};