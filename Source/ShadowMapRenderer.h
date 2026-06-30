#pragma once
#include "Common.h"
#include "Model.h"

class ShadowMapRenderer
{
public:
	ShadowMapRenderer(UINT shadowTexelSize);
	~ShadowMapRenderer() = default;

	// 開始処理
	void Begin(Scene* scene, const DirectX::XMFLOAT3& position);

	// 描画実行
	void Draw(Scene* scene, const Model* model);

	// 終了処理
	void End();

	// シェーダーリソースビュー取得
	ID3D11ShaderResourceView* GetShaderResourceView() const { return shaderResourceView.Get(); }

	// サンプラステート取得
	ID3D11SamplerState* GetSamplerState() const { return samplerState.Get(); }

	// ライトビュープロジェクション行列取得
	const Matrix& GetLightViewProjection() const { return lightViewProjection; }

	const UINT& GetShadowTexelSize() const { return shadowTexelSize; }

	void RemakeTextures();

	// シリアライズ/デシリアライズ
	void Serialize(json& j);
	void Deserialize(json& j);

private:

	friend class Editor;
	void OnInspector();

	static const uint32_t MAX_BONES = 256;
	struct SKINNING_CONSTANT_BUFFER // b0
	{
		Matrix boneTransforms[MAX_BONES];
	};

	struct OBJECT_CONSTANT_BUFFER // b1
	{
		Matrix world;
	};

	struct SCENE_CONSTANT_BUFFER // b2
	{
		Matrix lightViewProjection;
	};

	Microsoft::WRL::ComPtr<ID3D11Buffer> skinningConstantBuffer; // b0
	Microsoft::WRL::ComPtr<ID3D11Buffer> objectConstantBuffer; // b1
	Microsoft::WRL::ComPtr<ID3D11Buffer> sceneConstantBuffer; // b2

	D3D11_VIEWPORT cachedViewport;
	Matrix lightViewProjection;
	UINT shadowTexelSize;

	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> cachedRenderTargetView;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> cachedDepthStencilView;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shaderResourceView;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthStencilView;

	Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;

	Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerState;
};