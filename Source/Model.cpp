#include "Misc.h"
#include "AssimpImporter.h"
#include "Model.h"
#include <filesystem>
#include "GpuResourceUtils.h"

// コンストラクタ
Model::Model(ID3D11Device* device, const char* filename)
{
	std::filesystem::path filepath(filename);
	std::filesystem::path dirpath(filepath.parent_path()); // ディレクトリパスを取得

	// ファイル読み込み
	AssimpImporter importer(filename);

	// ノードデータ読み取り
	importer.LoadNodes(nodes);

	// メッシュデータ読み取り
	importer.LoadMeshes(meshes, nodes);

	// ノード構築
	for (size_t nodeIndex = 0; nodeIndex < nodes.size(); ++nodeIndex)
	{
		Node& node = nodes.at(nodeIndex);
		// 親子関係を構築
		node.parent = node.parentIndex >= 0 ? &nodes.at(node.parentIndex) : nullptr;
		if (node.parent != nullptr)
		{
			node.parent->children.emplace_back(&node);
		}
	}

	// マテリアルデータ読み取り
	importer.LoadMaterials(materials);

	// マテリアル構築
	for (Material& material : materials)
	{
		if (material.diffuseTextureFileName.empty())
		{
			// ダミーテクスチャ作成
			HRESULT hr = GpuResourceUtils::CreateDummyTexture(device, 0xFFFFFFFF,
				material.diffuseMap.GetAddressOf());
			_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
		}
		else
		{
			// ディフューズテクスチャ読み込み
			// 相対パスを解決し、テクスチャファイルパスを作成
			std::filesystem::path diffuseTexturePath(dirpath / material.diffuseTextureFileName);
			HRESULT hr = GpuResourceUtils::LoadTexture(device, diffuseTexturePath.string().c_str(),
				material.diffuseMap.GetAddressOf());
			_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
		}
	}

	// メッシュ構築
	for (Mesh& mesh : meshes)
	{
		// 参照ノード設定
		// ノードインデックスデータから参照しやすいようにポインタを設定する
		mesh.node = &nodes.at(mesh.nodeIndex);

		// 参照マテリアル設定
		// メッシュデータからアクセスしやすいようにマテリアルのポインタを保持する
		mesh.material = &materials.at(mesh.materialIndex);

		// ボーン構築
		for (Bone& bone : mesh.bones)
		{
			// 参照ノード設定
			bone.node = &nodes.at(bone.nodeIndex);
		}

		// 頂点バッファ
		{
			D3D11_BUFFER_DESC bufferDesc = {};
			D3D11_SUBRESOURCE_DATA subresourceData = {};
			bufferDesc.ByteWidth = static_cast<UINT>(sizeof(Vertex) * mesh.vertices.size());
			bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
			bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			bufferDesc.CPUAccessFlags = 0;
			bufferDesc.MiscFlags = 0;
			bufferDesc.StructureByteStride = 0;
			subresourceData.pSysMem = mesh.vertices.data();
			subresourceData.SysMemPitch = 0;
			subresourceData.SysMemSlicePitch = 0;
			HRESULT hr = device->CreateBuffer(&bufferDesc, &subresourceData, mesh.vertexBuffer.GetAddressOf());
			_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
		}

		// インデックスバッファ
		{
			D3D11_BUFFER_DESC bufferDesc = {};
			D3D11_SUBRESOURCE_DATA subresourceData = {};
			bufferDesc.ByteWidth = static_cast<UINT>(sizeof(uint32_t) * mesh.indices.size());
			bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
			bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
			bufferDesc.CPUAccessFlags = 0;
			bufferDesc.MiscFlags = 0;
			bufferDesc.StructureByteStride = 0;
			subresourceData.pSysMem = mesh.indices.data();
			subresourceData.SysMemPitch = 0;
			subresourceData.SysMemSlicePitch = 0;
			HRESULT hr = device->CreateBuffer(&bufferDesc, &subresourceData, mesh.indexBuffer.GetAddressOf());
			_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
		}
	}
}

// トランスフォーム更新処理
void Model::UpdateTransform(const DirectX::XMFLOAT4X4& worldTransform)
{
	for (Node& node : nodes)
	{
		// ローカル行列算出
		DirectX::XMMATRIX S = DirectX::XMMatrixScaling(node.scale.x, node.scale.y, node.scale.z);
		DirectX::XMMATRIX R = DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&node.rotation));
		DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(node.position.x, node.position.y, node.position.z);
		DirectX::XMMATRIX LocalTransform = S * R * T;

		// ワールド行列算出
		DirectX::XMMATRIX ParentWorldTransform;
		if (node.parent != nullptr)
		{
			ParentWorldTransform = DirectX::XMLoadFloat4x4(&node.parent->worldTransform);
		}
		else
		{
			ParentWorldTransform = DirectX::XMLoadFloat4x4(&worldTransform);
		}
		DirectX::XMMATRIX WorldTransform = LocalTransform * ParentWorldTransform;

		// 計算結果を格納
		DirectX::XMStoreFloat4x4(&node.localTransform, LocalTransform);
		DirectX::XMStoreFloat4x4(&node.worldTransform, WorldTransform);
	}
}