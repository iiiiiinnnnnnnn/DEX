#pragma once

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include "Model.h"

class AssimpImporter
{
private:
	using MeshList = std::vector<Model::Mesh>;
public:
	AssimpImporter(const char* filename);

	// メッシュデータを読み込み
	void LoadMeshes(MeshList& meshes);

private:
	// aiVector3D → XMFLOAT3
	static DirectX::XMFLOAT3 AssimpImporter::aiVector3DToXMFLOAT3(const aiVector3D& aValue);

private:
	Assimp::Importer aImporter;
	const aiScene* aScene = nullptr;
};