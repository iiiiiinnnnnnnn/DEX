#include "Misc.h"
#include "AssimpImporter.h"
#include "Model.h"
#include <filesystem>
#include "GpuResourceUtils.h"

// コンストラクタ
Model::Model(ID3D11Device* device, const char* filename, float scaling)
	: scaling(scaling)
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

		if (material.normalTextureFileName.empty())
		{
			// 法線ベクトルが(0,0,1)になるダミーテクスチャを作成
			HRESULT hr = GpuResourceUtils::CreateDummyTexture(device, 0xFFFF7F7F,
				material.normalMap.GetAddressOf());
			_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
		}
		else
		{
			// 法線テクスチャ読み込み
			std::filesystem::path texturePath(dirpath / material.normalTextureFileName);
			HRESULT hr = GpuResourceUtils::LoadTexture(device, texturePath.string().c_str(),
				material.normalMap.GetAddressOf());
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

	// アニメーションデータ読み取り
	importer.LoadAnimations(animations, nodes);

	// ノードキャッシュ
	nodeCaches.resize(nodes.size());
}

// トランスフォーム更新処理
void Model::UpdateTransform(const DirectX::XMFLOAT4X4& worldTransform)
{
	DirectX::XMMATRIX ParentWorldTransform = DirectX::XMLoadFloat4x4(&worldTransform);

	// 右手座標系から左手座標系へ変換する行列
	DirectX::XMMATRIX CoordinateSystemTransform = DirectX::XMMatrixScaling(-scaling, scaling, scaling);

	for (Node& node : nodes)
	{
		// ローカル行列算出
		DirectX::XMMATRIX S = DirectX::XMMatrixScaling(node.scale.x, node.scale.y, node.scale.z);
		DirectX::XMMATRIX R = DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&node.rotation));
		DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(node.position.x, node.position.y, node.position.z);
		DirectX::XMMATRIX LocalTransform = S * R * T;

		// グローバル行列算出
		DirectX::XMMATRIX ParentGlobalTransform;
		if (node.parent != nullptr)
		{
			ParentGlobalTransform = DirectX::XMLoadFloat4x4(&node.parent->globalTransform);
		}
		else
		{
			ParentGlobalTransform = DirectX::XMMatrixIdentity();
		}
		DirectX::XMMATRIX GlobalTransform = LocalTransform * ParentGlobalTransform;

		// ワールド行列算出
		DirectX::XMMATRIX WorldTransform = GlobalTransform * CoordinateSystemTransform * ParentWorldTransform;

		// 計算結果を格納
		DirectX::XMStoreFloat4x4(&node.localTransform, LocalTransform);
		DirectX::XMStoreFloat4x4(&node.globalTransform, GlobalTransform);
		DirectX::XMStoreFloat4x4(&node.worldTransform, WorldTransform);
	}
}

// アニメーション再生
void Model::PlayAnimation(int index, bool loop, float blendSeconds)
{
	currentAnimationIndex = index;
	currentAnimationSeconds = 0;
	animationLoop = loop;
	animationPlaying = true;

	// ブレンドパラメータ
	animationBlending = blendSeconds > 0.0f;
	currentAnimationBlendSeconds = 0.0f;
	animationBlendSecondsLength = blendSeconds;

	// 現在の姿勢をキャッシュする
	for (size_t i = 0; i < nodes.size(); ++i)
	{
		const Node& src = nodes.at(i);
		NodeCache& dst = nodeCaches.at(i);
		dst.position = src.position;
		dst.rotation = src.rotation;
		dst.scale = src.scale;
	}
}

// アニメーション再生中か
bool Model::IsPlayAnimation() const
{
	if (currentAnimationIndex < 0) return false;
	if (currentAnimationIndex >= animations.size()) return false;
	return animationPlaying;
}

// アニメーション更新処理
void Model::UpdateAnimation(float elapsedTime)
{
	ComputeAnimation(elapsedTime);
	ComputeBlending(elapsedTime);
}

// アニメーション計算処理
void Model::ComputeAnimation(float elapsedTime)
{
	if (!IsPlayAnimation()) return;

	// 指定のアニメーションデータを取得
	const Animation& animation = animations.at(currentAnimationIndex);

	// ノード毎のアニメーション処理
	for (size_t nodeIndex = 0; nodeIndex < animation.nodeAnims.size(); ++nodeIndex)
	{
		Node& node = nodes.at(nodeIndex);
		const NodeAnim& nodeAnim = animation.nodeAnims.at(nodeIndex);

		// 位置
		for (size_t index = 0; index < nodeAnim.positionKeyframes.size() - 1; ++index)
		{
			// 現在の時間がどのキーフレームの間にいるか判定する
			const VectorKeyframe& keyframe0 = nodeAnim.positionKeyframes.at(index);
			const VectorKeyframe& keyframe1 = nodeAnim.positionKeyframes.at(index + 1);
			if (currentAnimationSeconds >= keyframe0.seconds && currentAnimationSeconds < keyframe1.seconds)
			{
				// 再生時間とキーフレームの時間から補完率を算出する
				float rate = (currentAnimationSeconds - keyframe0.seconds) / (keyframe1.seconds - keyframe0.seconds);

				// 前のキーフレームと次のキーフレームの姿勢を補完
				DirectX::XMVECTOR V0 = DirectX::XMLoadFloat3(&keyframe0.value);
				DirectX::XMVECTOR V1 = DirectX::XMLoadFloat3(&keyframe1.value);
				DirectX::XMVECTOR V = DirectX::XMVectorLerp(V0, V1, rate);

				// 計算結果をノードに格納
				DirectX::XMStoreFloat3(&node.position, V);
			}
		}

		// 回転
		for (size_t index = 0; index < nodeAnim.rotationKeyframes.size() - 1; ++index)
		{
			// 現在の時間がどのキーフレームの間にいるか判定する
			const QuaternionKeyframe& keyframe0 = nodeAnim.rotationKeyframes.at(index);
			const QuaternionKeyframe& keyframe1 = nodeAnim.rotationKeyframes.at(index + 1);
			if (currentAnimationSeconds >= keyframe0.seconds && currentAnimationSeconds < keyframe1.seconds)
			{
				// 再生時間とキーフレームの時間から補完率を算出する
				float rate = (currentAnimationSeconds - keyframe0.seconds) / (keyframe1.seconds - keyframe0.seconds);

				// 前のキーフレームと次のキーフレームの姿勢を補完
				DirectX::XMVECTOR Q0 = DirectX::XMLoadFloat4(&keyframe0.value);
				DirectX::XMVECTOR Q1 = DirectX::XMLoadFloat4(&keyframe1.value);
				DirectX::XMVECTOR Q = DirectX::XMQuaternionSlerp(Q0, Q1, rate);

				// 計算結果をノードに格納
				DirectX::XMStoreFloat4(&node.rotation, Q);
			}
		}

		// スケール
		for (size_t index = 0; index < nodeAnim.scaleKeyframes.size() - 1; ++index)
		{
			// 現在の時間がどのキーフレームの間にいるか判定する
			const VectorKeyframe& keyframe0 = nodeAnim.scaleKeyframes.at(index);
			const VectorKeyframe& keyframe1 = nodeAnim.scaleKeyframes.at(index + 1);
			if (currentAnimationSeconds >= keyframe0.seconds && currentAnimationSeconds < keyframe1.seconds)
			{
				// 再生時間とキーフレームの時間から補完率を算出する
				float rate = (currentAnimationSeconds - keyframe0.seconds) / (keyframe1.seconds - keyframe0.seconds);

				// 前のキーフレームと次のキーフレームの姿勢を補完
				DirectX::XMVECTOR V0 = DirectX::XMLoadFloat3(&keyframe0.value);
				DirectX::XMVECTOR V1 = DirectX::XMLoadFloat3(&keyframe1.value);
				DirectX::XMVECTOR V = DirectX::XMVectorLerp(V0, V1, rate);

				// 計算結果をノードに格納
				DirectX::XMStoreFloat3(&node.scale, V);
			}
		}
	}
	// 時間経過
	currentAnimationSeconds += elapsedTime;

	// 再生時間が終端時間を超えたら
	if (currentAnimationSeconds >= animation.secondsLength)
	{
		if (animationLoop)
		{
			// 再生時間を巻き戻す
			currentAnimationSeconds -= animation.secondsLength;
		}
		else
		{
			// 再生終了時間にする
			currentAnimationSeconds = animation.secondsLength;
			animationPlaying = false;
		}
	}
}

// ブレンディング計算処理
void Model::ComputeBlending(float elapsedTime)
{
	if (!animationBlending) return;

	// ブレンド率の計算
	float rate = currentAnimationBlendSeconds / animationBlendSecondsLength;

	// ブレンド計算
	int count = static_cast<int>(nodes.size());
	for (int i = 0; i < count; ++i)
	{
		const NodeCache& cache = nodeCaches.at(i);
		Node& node = nodes.at(i);
		DirectX::XMVECTOR S0 = DirectX::XMLoadFloat3(&cache.scale);
		DirectX::XMVECTOR S1 = DirectX::XMLoadFloat3(&node.scale);
		DirectX::XMVECTOR R0 = DirectX::XMLoadFloat4(&cache.rotation);
		DirectX::XMVECTOR R1 = DirectX::XMLoadFloat4(&node.rotation);
		DirectX::XMVECTOR T0 = DirectX::XMLoadFloat3(&cache.position);
		DirectX::XMVECTOR T1 = DirectX::XMLoadFloat3(&node.position);
		DirectX::XMVECTOR S = DirectX::XMVectorLerp(S0, S1, rate);
		DirectX::XMVECTOR R = DirectX::XMQuaternionSlerp(R0, R1, rate);
		DirectX::XMVECTOR T = DirectX::XMVectorLerp(T0, T1, rate);
		DirectX::XMStoreFloat3(&node.scale, S);
		DirectX::XMStoreFloat4(&node.rotation, R);
		DirectX::XMStoreFloat3(&node.position, T);
	}

	// 時間経過
	currentAnimationBlendSeconds += elapsedTime;
	if (currentAnimationBlendSeconds >= animationBlendSecondsLength)
	{
		currentAnimationBlendSeconds = animationBlendSecondsLength;
		animationBlending = false;
	}
}