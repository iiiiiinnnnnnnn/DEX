#pragma once

#include <Common.h>

class Scene;

// スプライト
class Sprite
{
public:
	Sprite(ID3D11Device* device);
	Sprite(ID3D11Device* device, std::string filename);

	// 頂点データ
	struct Vertex
	{
		Vector3 position;
		Color color;
		Vector2 texcoord;
	};

	// 描画実行
	void Render(Scene* scene,
		Vector3 dxyz, // 左上位置 // 奥行
		Vector2 dwh, // 幅、高さ
		Vector2 sxy, // 画像切り抜き位置
		Vector2 swh, // 画像切り抜きサイズ
		float angle, // 角度
		Color rgba) const;

	void Render(Scene* scene,
		Vector3 dxyz, // 左上位置 // 奥行
		Vector2 dwh, // 幅、高さ
		float angle = 0, // 角度
		Color rgba = {1, 1, 1, 1}) const;

	ID3D11ShaderResourceView* GetSRV() {
		return shaderResourceView.Get();
	}

private:
	Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shaderResourceView;

	float textureWidth = 0;
	float textureHeight = 0;
};