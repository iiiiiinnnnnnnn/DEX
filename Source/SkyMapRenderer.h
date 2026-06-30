#pragma once

#include <Common.h>

class Scene;

class SkyMapRenderer {
private:

	// 頂点データ
	struct Vertex
	{
		Vector3 position;
		Color color;
		Vector2 texcoord;
	};
	struct CbSkyMap
	{
		Matrix inverseViewProjection;
		Vector3 cameraPosition;
		float _dummy;
	};
	Microsoft::WRL::ComPtr<ID3D11Buffer> skyMapConstantBuffer;

	Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;

	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shaderResourceView;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthStencilState;

	float textureWidth = 0;
	float textureHeight = 0;

	std::string texturePath;

	friend class Editor;
	void OnInspector();

public:
	SkyMapRenderer();

	DEFINE_TYPE(ICON_FA_CLOUD, スカイマップ)

	void Render(Scene* scene);

	void LoadPanorama(const std::string& path);

	// シェーダーリソースビュー取得
	ID3D11ShaderResourceView* GetShaderResourceView() const { return shaderResourceView.Get(); }

	void Serialize(json& j) {
		j["texturePath"] = texturePath;
	}

	void Deserialize(const json& j) {
		texturePath = j["texturePath"];
		LoadPanorama(texturePath);
	}
};