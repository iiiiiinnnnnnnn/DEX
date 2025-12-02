#pragma once

#include <wrl.h>
#include <d3d11.h>
#include <DirectXMath.h>

// スプライト
class Sprite
{
public:
	Sprite(ID3D11Device* device);

	// 頂点データ
	struct Vertex
	{
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT4 color;
	};

	// 描画実行
	void Render(ID3D11DeviceContext* dc,
		float dx, float dy, // 左上位置
		float dw, float dh, // 幅、高さ
		float angle = 0, // 角度
		float r = 1, float g = 1, float b = 1, float a = 1 // 色
	) const;

private:
	Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
};