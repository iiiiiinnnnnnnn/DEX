#include "Misc.h"
#include "RenderState.h"

// コンストラクタ
RenderState::RenderState(ID3D11Device* device)
{
	// ポイントサンプリング＆テクスチャ繰り返しあり
	{
		D3D11_SAMPLER_DESC desc;
		desc.MipLODBias = 0.0f;
		desc.MaxAnisotropy = 1;
		desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		desc.MinLOD = -D3D11_FLOAT32_MAX;
		desc.MaxLOD = D3D11_FLOAT32_MAX;
		desc.BorderColor[0] = 1.0f;
		desc.BorderColor[1] = 1.0f;
		desc.BorderColor[2] = 1.0f;
		desc.BorderColor[3] = 1.0f;
		desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
		HRESULT hr = device->CreateSamplerState(&desc,
			samplerStates[static_cast<int>(SamplerState::PointWrap)].GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
	}
	// ポイントサンプリング＆テクスチャ繰り返しなし
	{
		D3D11_SAMPLER_DESC desc;
		desc.MipLODBias = 0.0f;
		desc.MaxAnisotropy = 1;
		desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		desc.MinLOD = -D3D11_FLOAT32_MAX;
		desc.MaxLOD = D3D11_FLOAT32_MAX;
		desc.BorderColor[0] = 1.0f;
		desc.BorderColor[1] = 1.0f;
		desc.BorderColor[2] = 1.0f;
		desc.BorderColor[3] = 1.0f;
		desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
		HRESULT hr = device->CreateSamplerState(&desc,
			samplerStates[static_cast<int>(SamplerState::PointClamp)].GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
	}
	// リニアサンプリング＆テクスチャ繰り返しあり
	{
		D3D11_SAMPLER_DESC desc;
		desc.MipLODBias = 0.0f;
		desc.MaxAnisotropy = 1;
		desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		desc.MinLOD = -D3D11_FLOAT32_MAX;
		desc.MaxLOD = D3D11_FLOAT32_MAX;
		desc.BorderColor[0] = 1.0f;
		desc.BorderColor[1] = 1.0f;
		desc.BorderColor[2] = 1.0f;
		desc.BorderColor[3] = 1.0f;
		desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		HRESULT hr = device->CreateSamplerState(&desc,
			samplerStates[static_cast<int>(SamplerState::LinearWrap)].GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
	}
	// リニアサンプリング＆テクスチャ繰り返しなし
	{
		D3D11_SAMPLER_DESC desc;
		desc.MipLODBias = 0.0f;
		desc.MaxAnisotropy = 1;
		desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		desc.MinLOD = -D3D11_FLOAT32_MAX;
		desc.MaxLOD = D3D11_FLOAT32_MAX;
		desc.BorderColor[0] = 1.0f;
		desc.BorderColor[1] = 1.0f;
		desc.BorderColor[2] = 1.0f;
		desc.BorderColor[3] = 1.0f;
		desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		HRESULT hr = device->CreateSamplerState(&desc,
			samplerStates[static_cast<int>(SamplerState::LinearClamp)].GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
	}
}