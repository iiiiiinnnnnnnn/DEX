#pragma once

#include <Common.h>

class Scene;

class GizmosRenderer
{
public:
	GizmosRenderer();
	~GizmosRenderer() {}

	// 箱描画
	void DrawBox(
		const Vector3& position,
		const Quaternion& rotation,
		const Vector3& size,
		const Color& color);

	// 球描画
	void DrawSphere(
		const Vector3& position,
		float radius,
		const Color& color);

	// 線描画
	void DrawLine(
		const Vector3& a,
		const Vector3& b,
		const Color& color);

	// 描画実行
	void Render(Scene* scene);

private:
	struct Mesh
	{
		Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
		UINT vertexCount;
	};

	struct Instance
	{
		Mesh* mesh;
		Matrix worldTransform;
		Color color;
	};

	struct CbMesh
	{
		Matrix worldViewProjection;
		Color color;
	};

	// メッシュ生成
	void CreateMesh(ID3D11Device* device, const std::vector<Vector3>& vertices, Mesh& mesh);

	// 箱メッシュ作成
	void CreateBoxMesh(ID3D11Device* device, float width, float height, float depth);

	// 球メッシュ作成
	void CreateSphereMesh(ID3D11Device* device, float radius, int subdivisions);

private:
	Mesh boxMesh;
	Mesh sphereMesh;
	Mesh lineMesh;
	std::vector<Instance> instances;

	Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;
	Microsoft::WRL::ComPtr<ID3D11Buffer> constantBuffer;
};