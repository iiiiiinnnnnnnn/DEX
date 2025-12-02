#include <fstream>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include "Misc.h"
#include "AssimpImporter.h"

// コンストラクタ
AssimpImporter::AssimpImporter(const char* filename)
	: filepath(filename)
{
	// 拡張子取得
	std::string extension = filepath.extension().string();
	std::transform(extension.begin(), extension.end(), extension.begin(), tolower); // 小文字化

	// FBXファイルの場合は特殊なインポートオプション設定をする
	if (extension == ".fbx")
	{
		// $AssimpFBX$が付加された余計なノードを作成してしまうのを抑制する
		aImporter.SetPropertyInteger(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
	}

	// インポート時のオプションフラグ
	uint32_t aFlags = aiProcess_Triangulate // 多角形を三角形化する
		| aiProcess_JoinIdenticalVertices // 重複頂点をマージする
		| aiProcess_PopulateArmatureData; // ボーンの参照データを取得できるようにする

	// ファイル読み込み
	aScene = aImporter.ReadFile(filename, aFlags);
	_ASSERT_EXPR_A(aScene, "3D Model File not found");
}

// メッシュデータを読み込み
void AssimpImporter::LoadMeshes(MeshList& meshes, const NodeList& nodes)
{
	LoadMeshes(meshes, nodes, aScene->mRootNode);
}
void AssimpImporter::LoadMeshes(MeshList& meshes, const NodeList& nodes, const aiNode* aNode)
{
	const aiMesh* aMesh = aScene->mMeshes[0];

	// メッシュデータ読み取り
	for (uint32_t aMeshIndex = 0; aMeshIndex < aNode->mNumMeshes; ++aMeshIndex)
	{
		const aiMesh* aMesh = aScene->mMeshes[aNode->mMeshes[aMeshIndex]];

		// メッシュデータ格納
		Model::Mesh& mesh = meshes.emplace_back();
		mesh.nodeIndex = nodeIndexMap[aNode];
		mesh.vertices.resize(aMesh->mNumVertices);
		mesh.indices.resize(aMesh->mNumFaces * 3);
		mesh.materialIndex = static_cast<int>(aMesh->mMaterialIndex);

		// 頂点データ
		for (uint32_t aVertexIndex = 0; aVertexIndex < aMesh->mNumVertices; ++aVertexIndex)
		{
			Model::Vertex& vertex = mesh.vertices.at(aVertexIndex);

			// 位置
			if (aMesh->HasPositions())
			{
				vertex.position = aiVector3DToXMFLOAT3(aMesh->mVertices[aVertexIndex]);
			}

			// テクスチャ座標
			if (aMesh->HasTextureCoords(0))
			{
				vertex.texcoord = aiVector3DToXMFLOAT2(aMesh->mTextureCoords[0][aVertexIndex]);
				// OpenGL と DirectX で縦方向のテクスチャ座標が違う。Assimp は OpenGL 基準のデータなので変換する
				vertex.texcoord.y = 1.0f - vertex.texcoord.y;
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

		// スキニングデータ
		if (aMesh->mNumBones > 0)
		{
			// ボーン影響力データ
			struct BoneInfluence
			{
				uint32_t indices[4] = { 0, 0, 0, 0 };
				float weights[4] = { 1, 0, 0, 0 };
				int useCount = 0;
				void Add(uint32_t index, float weight)
				{
					if (useCount >= 4) return;
					for (int i = 0; i < useCount; ++i)
					{
						if (indices[i] == index)
						{
							return;
						}
					}
					indices[useCount] = index;
					weights[useCount] = weight;
					useCount++;
				}
			};
			std::vector<BoneInfluence> boneInfluences;
			boneInfluences.resize(aMesh->mNumVertices);

			// メッシュに影響するボーンデータを収集する
			for (uint32_t aBoneIndex = 0; aBoneIndex < aMesh->mNumBones; ++aBoneIndex)
			{
				const aiBone* aBone = aMesh->mBones[aBoneIndex];

				// 頂点影響力データを抽出
				for (uint32_t aWightIndex = 0; aWightIndex < aBone->mNumWeights; ++aWightIndex)
				{
					const aiVertexWeight& aWeight = aBone->mWeights[aWightIndex];
					BoneInfluence& boneInfluence = boneInfluences.at(aWeight.mVertexId);
					boneInfluence.Add(aBoneIndex, aWeight.mWeight);
				}

				// ボーンデータ取得
				Model::Bone& bone = mesh.bones.emplace_back();
				bone.nodeIndex = nodeIndexMap[aBone->mNode];
				bone.offsetTransform = aiMatrix4x4ToXMFLOAT4X4(aBone->mOffsetMatrix);
			}

			// 頂点影響力データを格納
			for (size_t vertexIndex = 0; vertexIndex < mesh.vertices.size(); ++vertexIndex)
			{
				Model::Vertex& vertex = mesh.vertices.at(vertexIndex);
				BoneInfluence& boneInfluence = boneInfluences.at(vertexIndex);
				vertex.boneWeight.x = boneInfluence.weights[0];
				vertex.boneWeight.y = boneInfluence.weights[1];
				vertex.boneWeight.z = boneInfluence.weights[2];
				vertex.boneWeight.w = boneInfluence.weights[3];
				vertex.boneIndex.x = boneInfluence.indices[0];
				vertex.boneIndex.y = boneInfluence.indices[1];
				vertex.boneIndex.z = boneInfluence.indices[2];
				vertex.boneIndex.w = boneInfluence.indices[3];
			}
		}
	}

	// 再帰的に子ノードを処理する
	for (uint32_t aNodeIndex = 0; aNodeIndex < aNode->mNumChildren; ++aNodeIndex)
	{
		LoadMeshes(meshes, nodes, aNode->mChildren[aNodeIndex]);
	}
}

// マテリアルデータを読み込み
void AssimpImporter::LoadMaterials(MaterialList& materials)
{
	// ディレクトリパス取得
	std::filesystem::path dirpath(filepath.parent_path());

	materials.resize(aScene->mNumMaterials);
	for (uint32_t aMaterialIndex = 0; aMaterialIndex < aScene->mNumMaterials; ++aMaterialIndex)
	{
		const aiMaterial* aMaterial = aScene->mMaterials[aMaterialIndex];
		Model::Material& material = materials.at(aMaterialIndex);

		// マテリアル名
		aiString aMaterialName;
		aMaterial->Get(AI_MATKEY_NAME, aMaterialName);
		material.name = aMaterialName.C_Str();

		// ディフューズ色
		aiColor3D aDiffuseColor;
		if (AI_SUCCESS == aMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, aDiffuseColor))
		{
			material.color = aiColor3DToXMFLOAT4(aDiffuseColor);
		}

		// テクスチャ読み込み関数
		auto loadTexture = [&](aiTextureType aTextureType, std::string& textureFilename)
			{
				// テクスチャファイルパス取得
				aiString aTextureFilePath;
				if (AI_SUCCESS == aMaterial->GetTexture(aTextureType, 0, &aTextureFilePath))
				{
					// 埋め込みテクスチャか確認
					const aiTexture* aTexture = aScene->GetEmbeddedTexture(aTextureFilePath.C_Str());
					if (aTexture != nullptr)
					{
						// テクスチャファイルパス作成
						std::filesystem::path textureFilePath(aTextureFilePath.C_Str());
						if (textureFilePath == "*0")
						{
							// テクスチャファイル名がなかった場合はマテリアル名とテクスチャタイプから作成
							textureFilePath = material.name + "_" + aiTextureTypeToString(aTextureType)
								+ "." + aTexture->achFormatHint;
						}
						textureFilePath = "Textures" / textureFilePath.filename();

						// 埋め込みテクスチャを出力するディレクトリを確認
						std::filesystem::path outputDirPath(dirpath / textureFilePath.parent_path());
						if (!std::filesystem::exists(outputDirPath))
						{
							// なかったらディレクトリ作成
							std::filesystem::create_directories(outputDirPath);
						}

						// 出力ディレクトリに画像ファイルを保存
						std::filesystem::path outputFilePath(dirpath / textureFilePath);
						if (!std::filesystem::exists(outputFilePath))
						{
							// mHeightが0の場合は画像の生データなのでそのままバイナリ出力
							if (aTexture->mHeight == 0)
							{
								std::ofstream os(outputFilePath.string().c_str(), std::ios::binary);
								os.write(reinterpret_cast<char*>(aTexture->pcData), aTexture->mWidth);
							}
							else
							{
								// リニアな画像データは.pngで出力
								outputFilePath.replace_extension(".png");
								stbi_write_png(
									outputFilePath.string().c_str(),
									static_cast<int>(aTexture->mWidth),
									static_cast<int>(aTexture->mHeight),
									static_cast<int>(sizeof(uint32_t)),
									aTexture->pcData, 0);
							}
						}

						// テクスチャファイルパスを格納
						textureFilename = textureFilePath.string();
					}
					else
					{
						// テクスチャファイルパスをそのまま格納
						textureFilename = aTextureFilePath.C_Str();
					}
				}
			};

		// ディフューズマップ
		loadTexture(aiTextureType_DIFFUSE, material.diffuseTextureFileName);
	}
}

// ノードデータを読み込み
void AssimpImporter::LoadNodes(NodeList& nodes)
{
	// 先頭のノードから順に処理していく
	LoadNodes(nodes, aScene->mRootNode, -1);
}
// ノードデータを再帰読み込み
void AssimpImporter::LoadNodes(NodeList& nodes, const aiNode* aNode, int parentIndex)
{
	// aiNode*からModel::Nodeのインデックスを取得できるようにする
	std::map<const aiNode*, int>::iterator it = nodeIndexMap.find(aNode);
	if (it == nodeIndexMap.end())
	{
		nodeIndexMap[aNode] = static_cast<int>(nodes.size());
	}
	// トランスフォームデータ取り出し
	aiVector3D aScale, aPosition;
	aiQuaternion aRotation;
	aNode->mTransformation.Decompose(aScale, aRotation, aPosition);
	// ノードデータ格納
	Model::Node& node = nodes.emplace_back();
	node.name = aNode->mName.C_Str();
	node.parentIndex = parentIndex;
	node.scale = aiVector3DToXMFLOAT3(aScale);
	node.rotation = aiQuaternionToXMFLOAT4(aRotation);
	node.position = aiVector3DToXMFLOAT3(aPosition);
	parentIndex = static_cast<int>(nodes.size() - 1);
	// 再帰的に子ノードを処理する
	for (uint32_t aNodeIndex = 0; aNodeIndex < aNode->mNumChildren; ++aNodeIndex)
	{
		LoadNodes(nodes, aNode->mChildren[aNodeIndex], parentIndex);
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
// aiColor3D → XMFLOAT4
DirectX::XMFLOAT4 AssimpImporter::AssimpImporter::aiColor3DToXMFLOAT4(const aiColor3D& aValue)
{
	return DirectX::XMFLOAT4(
		static_cast<float>(aValue.r),
		static_cast<float>(aValue.g),
		static_cast<float>(aValue.b),
		1.0f
	);
}
// aiVector3D → XMFLOAT2
DirectX::XMFLOAT2 AssimpImporter::AssimpImporter::aiVector3DToXMFLOAT2(const aiVector3D& aValue)
{
	return DirectX::XMFLOAT2(
		static_cast<float>(aValue.x),
		static_cast<float>(aValue.y)
	);
}
// aiQuaternion → XMFLOAT4
DirectX::XMFLOAT4 AssimpImporter::aiQuaternionToXMFLOAT4(const aiQuaternion& aValue)
{
	return DirectX::XMFLOAT4(
		static_cast<float>(aValue.x),
		static_cast<float>(aValue.y),
		static_cast<float>(aValue.z),
		static_cast<float>(aValue.w)
	);
}
// aiMatrix4x4 → XMFLOAT4X4
DirectX::XMFLOAT4X4 AssimpImporter::aiMatrix4x4ToXMFLOAT4X4(const aiMatrix4x4& aValue)
{
	return DirectX::XMFLOAT4X4(
		static_cast<float>(aValue.a1),
		static_cast<float>(aValue.b1),
		static_cast<float>(aValue.c1),
		static_cast<float>(aValue.d1),
		static_cast<float>(aValue.a2),
		static_cast<float>(aValue.b2),
		static_cast<float>(aValue.c2),
		static_cast<float>(aValue.d2),
		static_cast<float>(aValue.a3),
		static_cast<float>(aValue.b3),
		static_cast<float>(aValue.c3),
		static_cast<float>(aValue.d3),
		static_cast<float>(aValue.a4),
		static_cast<float>(aValue.b4),
		static_cast<float>(aValue.c4),
		static_cast<float>(aValue.d4)
	);
}