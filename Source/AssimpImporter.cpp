#include <fstream>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include "Misc.h"
#include "AssimpImporter.h"

// コンストラクタ
AssimpImporter::AssimpImporter(const char* filename)
	: filepath(filename)
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