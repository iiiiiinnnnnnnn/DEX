#include "Model.h"
#include "AssimpImporter.h"
#include "Graphics.h"
#include "GameObject.h"
#include <MaterialManager.h>
#include "Camera.h"
#include "Scene.h"
#include <AssetManager.h>

void Model::OnInspector()
{
	const char* preview = mdlpath.empty() ? "<none>" : mdlpath.c_str();

	if (ImGui::BeginCombo(("##" + mdlpath).c_str(), preview))
	{
		if (ImGui::Selectable("<none>", mdlpath.empty()))
		{
			mdlpath.clear();
			dirty = true;
		}

		AssetManager& assetManager = AssetManager::Instance();
		for (auto& dds : assetManager.ScanAssets(".mdl"))
		{
			std::string rel = dds.string();
			bool selected = (rel == mdlpath);

			if (ImGui::Selectable(rel.c_str(), selected))
			{
				mdlpath = rel;
				dirty = true;
			}
		}
		ImGui::EndCombo();
	}

	if (dirty)
	{
		// なぜかここクラッシュする。あとDeleteComponentも作らないとな
		GameObject* keepOwner = owner;

		*this = Model(mdlpath);

		owner = keepOwner;

		dirty = false;
	}

	if (ImGui::CollapsingHeader((std::string(ICON_FA_RUNNING) + (const char*)u8" アニメーション").c_str(), ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::TreePush(this);

		if (animations.empty()) {
			AppendAnimation("");
		}
		for (size_t i = 0; i < animations.size();)
		{
			const char* preview2 = animations[i].name.empty() ? "<none>" : animations[i].name.c_str();

			ImGui::Text("%d: ", i);
			ImGui::SameLine();
			// アニメーション再生
			/*if(ImGui::Button((std::string(ICON_FA_PLAY) + "##play" + std::to_string(i)).c_str(), ImVec2(30, 30)))
			{
				PlayAnimation(i, true);
			}*/
			ImGui::SameLine();

			if (ImGui::BeginCombo(("##" + std::to_string(i)).c_str(), preview2))
			{
				if (ImGui::Selectable("<none>", animations[i].name.empty()))
				{
					if (!animations[i].name.empty())
					{
						animations[i] = Animation::LoadFromFilePath("");
						animations[i].name = "";
					}
				}

				AssetManager& assetManager = AssetManager::Instance();

				for (auto& file : assetManager.ScanAssets(".anim"))
				{
					std::string rel = file.string();
					bool selected = (rel == animations[i].name);

					if (ImGui::Selectable(rel.c_str(), selected))
					{
						animations[i] = Animation::LoadFromFilePath(rel);
						animations[i].name = rel;
					}

					if (selected)
						ImGui::SetItemDefaultFocus();
				}

				ImGui::EndCombo();
			}

			ImGui::SameLine();

			if (ImGui::Button((std::string(ICON_FA_PLUS) + "##add" + std::to_string(i)).c_str(), ImVec2(30, 30)))
			{
				ImGui::OpenPopup("AddAnimationPopup");
			}

			ImGui::SameLine();

			if (ImGui::Button((std::string(ICON_FA_TRASH) + "##del" + std::to_string(i)).c_str(), ImVec2(30, 30)))
			{
				animations.erase(animations.begin() + i);
				continue;
			}

			++i;
		}

		if (ImGui::BeginPopup("AddAnimationPopup"))
		{
			AssetManager& assetManager = AssetManager::Instance();
			for (auto& dds : assetManager.ScanAssets(".anim"))
			{
				std::string rel = dds.string();
				bool selected = (rel == mdlpath);

				if (ImGui::MenuItem(rel.c_str()))
				{
					AppendAnimation(rel);
				}
			}

			ImGui::EndPopup();
		}

		ImGui::TreePop();
	}
}

// コンストラクタ
Model::Model(const std::string& mdlpath) : mdlpath(mdlpath)
{
	CtorMdl();
}

void Model::CtorMdl()
{
	auto device = Graphics::Instance().GetDevice();

	if (!std::filesystem::exists(mdlpath)) {
		LOG("モデルファイルがありません");
		isValid = false;
		return;
	}

	// デシリアライズする
	std::ifstream istream(mdlpath, std::ios::binary);
	if (istream.is_open())
	{
		cereal::BinaryInputArchive archive(istream);
		try {
			archive(
				CEREAL_NVP(nodes),
				CEREAL_NVP(meshes),
				CEREAL_NVP(materialSlotNames),
				CEREAL_NVP(materialPathes)
			);
		}
		catch (...)
		{
			_ASSERT_EXPR_A(false, "Model deserialize failed.");
		}
	}
	else
	{
		_ASSERT_EXPR_A(false, "Model File not found.");
	}

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

	// マテリアルのテクスチャを読み込んでシェーダーリソースビューにする
	for (auto& path : materialPathes)
	{
		auto cache = MaterialManager::Instance().getCache(path);

		for (auto& [key, var] : cache.reflectedParams.getTextures())
		{
			if (var.path.empty()) continue;

			Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
			D3D11_TEXTURE2D_DESC desc{};

			HRESULT hr = GpuResourceUtils::LoadTexture(
				device,
				var.path,
				srv.GetAddressOf(),
				&desc
			);

			if (FAILED(hr))
				continue;

			// Model 側の map に格納
			textureSRVs[var.path] = srv;
		}
	}

	// メッシュ構築
	for (Mesh& mesh : meshes)
	{
		// 参照ノード設定
		// ノードインデックスデータから参照しやすいようにポインタを設定する
		mesh.node = &nodes.at(mesh.nodeIndex);

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

	// ノードキャッシュ
	nodeCaches.resize(nodes.size());

	// 入力レイアウト
	D3D11_INPUT_ELEMENT_DESC inputElementDesc[]
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT },
		{ "WEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT },
		{ "BONES", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT },
	};

	// 頂点シェーダー
	GpuResourceUtils::LoadVertexShader(device, "ModelVS._cso", inputElementDesc,
		_countof(inputElementDesc), inputLayout.GetAddressOf(), vertexShader.GetAddressOf());

	// オブジェクト用定数バッファ
	GpuResourceUtils::CreateConstantBuffer<SKINNING_CONSTANT_BUFFER>(device, skinningConstantBuffer.GetAddressOf());

	// オブジェクト用定数バッファ
	GpuResourceUtils::CreateConstantBuffer<OBJECT_CONSTANT_BUFFER>(device, objectConstantBuffer.GetAddressOf());

	// シーン用定数バッファ
	GpuResourceUtils::CreateConstantBuffer<SCENE_CONSTANT_BUFFER>(device, sceneConstantBuffer.GetAddressOf());

	// ライト用定数バッファ
	GpuResourceUtils::CreateConstantBuffer<LIGHT_CONSTANT_BUFFER>(device, lightConstantBuffer.GetAddressOf());
}

// トランスフォーム更新処理
void Model::UpdateTransform(const Transform& worldTransform)
{
	if (!IsValid()) return;

	Matrix ParentWorldTransform = worldTransform.GetMatrix();

	for (Node& node : nodes)
	{
		// ローカル行列算出
		Matrix LocalTransform =
			Matrix::CreateScale(node.scale) *
			Matrix::CreateFromQuaternion(node.rotation) *
			Matrix::CreateTranslation(node.position);

		// グローバル行列算出
		Matrix ParentGlobalTransform;
		if (node.parent != nullptr)
		{
			ParentGlobalTransform = node.parent->globalTransform;
		}
		else
		{
			ParentGlobalTransform = Matrix::Identity;
		}
		Matrix GlobalTransform = LocalTransform * ParentGlobalTransform;

		// ワールド行列算出
		Matrix WorldTransform = GlobalTransform * ParentWorldTransform;

		// 計算結果を格納
		node.localTransform = LocalTransform;
		node.globalTransform = GlobalTransform;
		node.worldTransform = WorldTransform;
	}
}

void Model::AppendAnimation(std::string path)
{
	animations.push_back(Animation::LoadFromFilePath(path));
}

// アニメーション再生
void Model::PlayAnimation(int index, bool loop, float blendSeconds)
{
	if (!IsValid()) return;

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
	if (!IsValid()) return;

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
				node.position = Vector3::Lerp(keyframe0.value, keyframe1.value, rate);
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
				node.rotation = Quaternion::Slerp(keyframe0.value, keyframe1.value, rate);
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
				node.scale = Vector3::Lerp(keyframe0.value, keyframe1.value, rate);
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

		node.scale = Vector3::Lerp(cache.scale, node.scale, rate);
		node.rotation = Quaternion::Slerp(cache.rotation, node.rotation, rate);
		node.position = Vector3::Lerp(cache.position, node.position, rate);
	}

	// 時間経過
	currentAnimationBlendSeconds += elapsedTime;
	if (currentAnimationBlendSeconds >= animationBlendSecondsLength)
	{
		currentAnimationBlendSeconds = animationBlendSecondsLength;
		animationBlending = false;
	}
}

void Model::Update(float elapsedTime)
{
	UpdateAnimation(elapsedTime);
}

void Model::Render(Scene* scene)
{
	if (!IsValid()) return;

	UpdateTransform(owner->GetWorldTransform());

	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();

	for (const Model::Mesh& mesh : GetMeshes())
	{
		// BEGIN

		// 定数バッファ定義
		std::vector<ID3D11Buffer*> constantBuffers =
		{
			skinningConstantBuffer.Get(),
			objectConstantBuffer.Get(),
			sceneConstantBuffer.Get(),
			lightConstantBuffer.Get(),
		};

		auto cache = MaterialManager::Instance().getCache(materialPathes[mesh.materialIndex]);

		// シェーダー設定
		dc->IASetInputLayout(inputLayout.Get());
		dc->VSSetShader(vertexShader.Get(), nullptr, 0);
		cache.Apply(constantBuffers); // PSSetShader+CBuffer

		// 定数バッファ適用
		dc->VSSetConstantBuffers(0, (UINT)constantBuffers.size(), constantBuffers.data());
		dc->PSSetConstantBuffers(0, (UINT)constantBuffers.size(), constantBuffers.data());

		// サンプラステート設定
		auto rs = scene->GetRenderState();
		auto smr = scene->GetShadowMapRenderer();
		ID3D11SamplerState* samplerStates[] =
		{
			rs->GetSamplerState(SamplerState::LinearWrap),
			smr->GetSamplerState(),
		};
		dc->PSSetSamplers(0, _countof(samplerStates), samplerStates);

		// レンダーステート設定
		const float blend_factor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		dc->OMSetBlendState(rs->GetBlendState(BlendState::Transparency), blend_factor, 0xFFFFFFFF);
		dc->OMSetDepthStencilState(rs->GetDepthStencilState(DepthState::TestAndWrite), 0);
		dc->RSSetState(rs->GetRasterizerState(RasterizerState::SolidCullBack));

		// スキニング用定数バッファ更新
		SKINNING_CONSTANT_BUFFER skinningConstantBufferData{};
		if (mesh.bones.size() > 0)
		{
			for (size_t i = 0; i < mesh.bones.size(); ++i)
			{
				const Model::Bone& bone = mesh.bones[i];
				skinningConstantBufferData.boneTransforms[i] = bone.offsetTransform * bone.node->globalTransform;
			}
		}
		else
		{
			skinningConstantBufferData.boneTransforms[0] = mesh.node->globalTransform;
		}

		dc->UpdateSubresource(skinningConstantBuffer.Get(), 0, 0, &skinningConstantBufferData, 0, 0);

		// オブジェクト用定数バッファ更新
		OBJECT_CONSTANT_BUFFER objectConstantBufferData{};
		objectConstantBufferData.world = owner->GetWorldTransform().GetMatrix();
		dc->UpdateSubresource(objectConstantBuffer.Get(), 0, 0, &objectConstantBufferData, 0, 0);

		// シーン用定数バッファ更新
		SCENE_CONSTANT_BUFFER sceneConstantBufferData{};
		auto mc = scene->GetMainCamera();
		Matrix V = mc->GetView();
		Matrix P = mc->GetProjection();
		sceneConstantBufferData.view_projection = V * P;
		sceneConstantBufferData.light_view_projection = smr->GetLightViewProjection();
		sceneConstantBufferData.shadowTexelSize = (float)smr->GetShadowTexelSize();
		sceneConstantBufferData.camera_position = mc->owner->GetWorldTransform().position;
		dc->UpdateSubresource(sceneConstantBuffer.Get(), 0, 0, &sceneConstantBufferData, 0, 0);

		// ライト用定数バッファ更新
		LIGHT_CONSTANT_BUFFER lightConstantBufferData{};
		// directionalLight
		{
			const DirectionalLight* directionalLight = scene->GetDirectionalLight();

			// forward を回す
			Vector3 dir = Vector3::TransformNormal(
				Vector3::Forward,
				Matrix::CreateFromQuaternion(
					directionalLight->owner->GetWorldTransform().rotation));
			dir.Normalize();
			lightConstantBufferData.directionLight.direction = dir;

			lightConstantBufferData.directionLight.color = directionalLight->color;
			lightConstantBufferData.directionLight.intensity = directionalLight->intensity;
		}

		// pointLight
		{
			const auto& pointLight = scene->GetPointLights();
			for (int i = 0; i < 10; i++)
			{
				if (pointLight.size() <= i) break;
				lightConstantBufferData.pointLight[i].position = pointLight[i]->owner->GetWorldTransform().position;
				lightConstantBufferData.pointLight[i].range = pointLight[i]->range;
				lightConstantBufferData.pointLight[i].color = pointLight[i]->color;
				lightConstantBufferData.pointLight[i].intensity = pointLight[i]->intensity;
			}
		}

		// spotLight
		{
			const auto& spotLight = scene->GetSpotLights();
			for (int i = 0; i < 10; i++)
			{
				if (spotLight.size() <= i) break;
				auto owner = spotLight[i]->owner;

				Vector3 dir = Vector3::Transform(Vector3::Forward, owner->GetWorldTransform().rotation);
				dir.Normalize();

				lightConstantBufferData.spotLight[i].direction = dir;
				lightConstantBufferData.spotLight[i].position = owner->GetWorldTransform().position;
				lightConstantBufferData.spotLight[i].color = spotLight[i]->color;
				lightConstantBufferData.spotLight[i].range = spotLight[i]->range;
				lightConstantBufferData.spotLight[i].innerCorn = spotLight[i]->innerCorn;
				lightConstantBufferData.spotLight[i].outerCorn = spotLight[i]->outerCorn;
				lightConstantBufferData.spotLight[i].intensity = spotLight[i]->intensity;
			}
		}

		// 定数バッファ更新
		dc->UpdateSubresource(lightConstantBuffer.Get(), 0, 0, &lightConstantBufferData, 0, 0);

		// DRAW

		// 頂点バッファ設定
		UINT stride = sizeof(Model::Vertex);
		UINT offset = 0;
		dc->IASetVertexBuffers(0, 1, mesh.vertexBuffer.GetAddressOf(), &stride, &offset);
		dc->IASetIndexBuffer(mesh.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
		dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// シェーダーリソースビュー設定

		// まず shadowMap (Material 管理外)
		ID3D11ShaderResourceView* shadowSrv = smr->GetShaderResourceView();
		dc->PSSetShaderResources(0, 1, &shadowSrv);

		// Material のテクスチャを slot 指定でセット
		for (auto& [name, binding] : cache.reflectedParams.getTextures())
		{
			if (binding.path.empty())
				continue;

			ID3D11ShaderResourceView* srv;

			// 何度もLoadTextureしないようにキャッシュ
			if (textureSRVs.contains(binding.path)) {
				srv = textureSRVs[binding.path].Get();
			}
			else
			{
				GpuResourceUtils::LoadTexture(Graphics::Instance().GetDevice(), binding.path, &srv);
				textureSRVs[binding.path] = srv;
			}

			dc->PSSetShaderResources(binding.slot, 1, &srv);
		}

		// 描画
		dc->DrawIndexed(static_cast<UINT>(mesh.indices.size()), 0, 0);
	}

	// END

	dc->VSSetShader(nullptr, nullptr, 0);
	dc->PSSetShader(nullptr, nullptr, 0);
	dc->IASetInputLayout(nullptr);

	// 設定されているシェーダーリソースを解除
	ID3D11ShaderResourceView* nullSrvs[16] = {};
	dc->PSSetShaderResources(0, 16, nullSrvs);
}
