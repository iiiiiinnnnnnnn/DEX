#pragma once

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
public:
	AssimpImporter(const char* filename);

	// メッシュデータを読み込み
	void LoadMeshes(MeshList& meshes);

	// マテリアルデータを読み込み
	void LoadMaterials(MaterialList& materials);

private:
	// aiVector3D → XMFLOAT3
	static DirectX::XMFLOAT3 AssimpImporter::aiVector3DToXMFLOAT3(const aiVector3D& aValue);
	// aiColor3D → XMFLOAT4
	static DirectX::XMFLOAT4 AssimpImporter::aiColor3DToXMFLOAT4(const aiColor3D& aValue);
	// aiVector3D → XMFLOAT2
	static DirectX::XMFLOAT2 AssimpImporter::aiVector3DToXMFLOAT2(const aiVector3D& aValue);

private:
	Assimp::Importer aImporter;
	const aiScene* aScene = nullptr;
	std::filesystem::path filepath;
};