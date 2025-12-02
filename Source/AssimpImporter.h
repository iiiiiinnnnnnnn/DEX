#pragma once

#include <map>
#include <filesystem>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include "Model.h"

class AssimpImporter
{
private:
	using MeshList = std::vector<Model::Mesh>;
	using MaterialList = std::vector<Model::Material>;
	using NodeList = std::vector<Model::Node>;
public:
	AssimpImporter(const char* filename);

	// メッシュデータを読み込み
	void LoadMeshes(MeshList& meshes, const NodeList& nodes);

	// マテリアルデータを読み込み
	void LoadMaterials(MaterialList& materials);

	// ノードデータを読み込み
	void LoadNodes(NodeList& nodes);

private:
	// メッシュデータを読み込み
	void LoadMeshes(MeshList& meshes, const NodeList& nodes, const aiNode* aNode);
	// ノードデータを再帰読み込み
	void LoadNodes(NodeList& nodes, const aiNode* aNode, int parentIndex);
	// aiVector3D → XMFLOAT3
	static DirectX::XMFLOAT3 AssimpImporter::aiVector3DToXMFLOAT3(const aiVector3D& aValue);
	// aiColor3D → XMFLOAT4
	static DirectX::XMFLOAT4 AssimpImporter::aiColor3DToXMFLOAT4(const aiColor3D& aValue);
	// aiVector3D → XMFLOAT2
	static DirectX::XMFLOAT2 AssimpImporter::aiVector3DToXMFLOAT2(const aiVector3D& aValue);
	// aiQuaternion → XMFLOAT4
	static DirectX::XMFLOAT4 aiQuaternionToXMFLOAT4(const aiQuaternion& aValue);

private:
	Assimp::Importer aImporter;
	const aiScene* aScene = nullptr;
	std::filesystem::path filepath;
	std::map<const aiNode*, int> nodeIndexMap;
};