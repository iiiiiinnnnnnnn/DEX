#pragma once

#include <vector>
#include <DirectXMath.h>
#include <wrl.h>
#include <d3d11.h>

class Model
{
public:
	Model(ID3D11Device* device, const char* filename);

	struct Vertex
	{
		DirectX::XMFLOAT3 position = { 0, 0, 0 };
	};

	struct Mesh
	{
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
	};

	// メッシュデータ取得
	const std::vector<Mesh>& GetMeshes() const { return meshes; }

private:
	std::vector<Mesh> meshes;
};