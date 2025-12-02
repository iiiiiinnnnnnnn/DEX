#include "Misc.h"
#include "AssimpImporter.h"

// コンストラクタ
AssimpImporter::AssimpImporter(const char* filename)
{
	// インポート時のオプションフラグ
	uint32_t aFlags = aiProcess_Triangulate // 多角形を三角形化する
		| aiProcess_JoinIdenticalVertices; // 重複頂点をマージする
	// ファイル読み込み
	aScene = aImporter.ReadFile(filename, aFlags);
	_ASSERT_EXPR_A(aScene, "3D Model File not found");
}

// メッシュデータを読み込み
void AssimpImporter::LoadMeshes(MeshList& meshes)
{
	const aiMesh* aMesh = aScene->mMeshes[0];
	// メッシュデータ格納
	Model::Mesh& mesh = meshes.emplace_back();
	mesh.vertices.resize(aMesh->mNumVertices);
	mesh.indices.resize(aMesh->mNumFaces * 3);
	// 頂点データ
	for (uint32_t aVertexIndex = 0; aVertexIndex < aMesh->mNumVertices; ++aVertexIndex)
	{
		Model::Vertex& vertex = mesh.vertices.at(aVertexIndex);
		// 位置
		if (aMesh->HasPositions())
		{
			vertex.position = aiVector3DToXMFLOAT3(aMesh->mVertices[aVertexIndex]);
		}
	}
	// インデックスデータ
	for (uint32_t aFaceIndex = 0; aFaceIndex < aMesh->mNumFaces; ++aFaceIndex)
	{
		const aiFace& aFace = aMesh->mFaces[aFaceIndex];
		uint32_t index = aFaceIndex * 3;
		mesh.indices[index + 0] = aFace.mIndices[0];
		mesh.indices[index + 1] = aFace.mIndices[1];
		mesh.indices[index + 2] = aFace.mIndices[2];
	}
}

// aiVector3D → XMFLOAT3
DirectX::XMFLOAT3 AssimpImporter::aiVector3DToXMFLOAT3(const aiVector3D & aValue)
{
	return DirectX::XMFLOAT3(
		static_cast<float>(aValue.x),
		static_cast<float>(aValue.y),
		static_cast<float>(aValue.z)
	);
}