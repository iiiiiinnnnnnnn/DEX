#pragma once

#include <Common.h>

class Scene;

class PostEffect
{
public:
	PostEffect(ID3D11Device* device);

	// 開始処理
	void Begin(Scene* scene);

	// 輝度抽出処理
	void LuminanceExtraction(Scene* scene, ID3D11ShaderResourceView* colorMap);

	// ブルーム処理
	void Bloom(Scene* scene, ID3D11ShaderResourceView* colorMap, ID3D11ShaderResourceView* luminanceMap);

	// 終了処理
	void End(Scene* scene);

	// デバッグGUI描画
	void OnInspector();

	// シリアライズ
	void Serialize(json& j) {
		j["luminanceExtractionLowerEdge"] = cbPostEffect.luminanceExtractionLowerEdge;
		j["luminanceExtractionHigherEdge"] = cbPostEffect.luminanceExtractionHigherEdge;
		j["gaussianSigma"] = cbPostEffect.gaussianSigma;
		j["bloomIntensity"] = cbPostEffect.bloomIntensity;
	}

	// デシリアライズ
	void Deserialize(json& j) {
		cbPostEffect.luminanceExtractionLowerEdge = j["luminanceExtractionLowerEdge"];
		cbPostEffect.luminanceExtractionHigherEdge = j["luminanceExtractionHigherEdge"];
		cbPostEffect.gaussianSigma = j["gaussianSigma"];
		cbPostEffect.bloomIntensity = j["bloomIntensity"];
	}

private:
	Microsoft::WRL::ComPtr<ID3D11VertexShader> fullscreenQuadVS;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> luminanceExtractionPS;

	struct CbPostEffect
	{
		float luminanceExtractionLowerEdge = 0.5f;
		float luminanceExtractionHigherEdge = 1.0f;
		float gaussianSigma = 10.0f;
		float bloomIntensity = 5.0f;
	};
	CbPostEffect cbPostEffect;
	Microsoft::WRL::ComPtr<ID3D11Buffer> constantBuffer;

	Microsoft::WRL::ComPtr<ID3D11PixelShader> bloomPS;
};