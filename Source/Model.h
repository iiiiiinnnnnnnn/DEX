#pragma once

#include <string>
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
		DirectX::XMFLOAT4 boneWeight = { 1, 0, 0, 0 };
		DirectX::XMUINT4 boneIndex = { 0, 0, 0, 0 };
		DirectX::XMFLOAT2 texcoord = { 0, 0 };
	};

	struct Material
	{
		std::string name;
		std::string diffuseTextureFileName;
		DirectX::XMFLOAT4 color = { 1, 1, 1, 1 };
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> diffuseMap;
	};

	struct Node
	{
		std::string name;
		int parentIndex;
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT4 rotation;
		DirectX::XMFLOAT3 scale;

		DirectX::XMFLOAT4X4 localTransform;
		DirectX::XMFLOAT4X4 worldTransform;
		Node* parent = nullptr;
		std::vector<Node*> children;
	};

	struct Bone
	{
		int nodeIndex;
		DirectX::XMFLOAT4X4 offsetTransform;
		Node* node = nullptr;
	};

	struct Mesh
	{
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
		int materialIndex = 0;
		Material* material = nullptr;
		int nodeIndex = 0;
		Node* node = nullptr;
		std::vector<Bone> bones;
	};

	// ルートノード取得
	Node* GetRootNode() { return nodes.data(); }

	// メッシュデータ取得
	const std::vector<Mesh>& GetMeshes() const { return meshes; }

	// マテリアルデータ取得
	const std::vector<Material>& GetMaterials() const { return materials; }

	// トランスフォーム更新処理
	void UpdateTransform(const DirectX::XMFLOAT4X4& worldTransform);

private:
	std::vector<Mesh> meshes;
	std::vector<Material> materials;
	std::vector<Node> nodes;
};