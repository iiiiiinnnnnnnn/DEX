#pragma once
#include "Shader.h"

class PhongShader : public Shader
{
public:
	PhongShader(ID3D11Device* device);
	~PhongShader() override = default;

	// ï`âÊäJén
	void Begin(const RenderContext& rc) override;

	// ÉÇÉfÉãï`âÊ
	void Draw(const RenderContext& rc, const Model* model) override;

	// ï`âÊèIóπ
	void End(const RenderContext& rc) override;

private:
	struct CbScene
	{
		DirectX::XMFLOAT4X4 viewProjection;
	};
	Microsoft::WRL::ComPtr<ID3D11Buffer> sceneConstantBuffer;

	struct CbMesh
	{
		DirectX::XMFLOAT4 materialColor;
	};
	Microsoft::WRL::ComPtr<ID3D11Buffer> meshConstantBuffer;

	struct CbSkeleton
	{
		DirectX::XMFLOAT4X4 boneTransforms[256];
	};
	Microsoft::WRL::ComPtr<ID3D11Buffer> skeletonConstantBuffer;

	Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;
};