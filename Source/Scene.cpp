#include "Scene.h"
#include "Graphics.h"
#include <imgui.h>
#include <functional>
#include "TransformUtils.h"

// コンストラクタ
SpriteTestScene::SpriteTestScene()
{
	ID3D11Device* device = Graphics::Instance().GetDevice();
	sprites[0] = std::make_unique<Sprite>(device, "Data/Sprite/player-sprites.png");
	sprites[1] = std::make_unique<Sprite>(device);
}

// 描画処理
void SpriteTestScene::Render(float elapsedTime)
{
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
	RenderState* renderState = Graphics::Instance().GetRenderState();
	ID3D11SamplerState* samplers[] =
	{
		renderState->GetSamplerState(SamplerState::PointClamp)
	};
	dc->PSSetSamplers(0, _countof(samplers), samplers);

	float width = 140;
	float height = 240;
	sprites[0]->Render(dc, 100, 50, 0, width, height, 10 * width, 3 * height, width, height, 0, 1, 1, 1, 1);
	sprites[0]->Render(dc, 100, 350, 0, 480, 300, 0, 1, 1, 1, 1);
	sprites[1]->Render(dc, 300, 50, 0, width, height, 0, 0, 1, 0, 1);
}



// コンストラクタ
DepthTestScene::DepthTestScene()
{
	ID3D11Device* device = Graphics::Instance().GetDevice();
	sprite = std::make_unique<Sprite>(device);
}

// 描画処理
void DepthTestScene::Render(float elapsedTime)
{
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
	RenderState* renderState = Graphics::Instance().GetRenderState();
	ID3D11SamplerState* samplers[] =
	{
	renderState->GetSamplerState(SamplerState::PointClamp)
	};
	dc->PSSetSamplers(0, _countof(samplers), samplers);

	// 深度書き込みなし＆深度テストなし
	dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::NoTestNoWrite), 0);
	sprite->Render(dc, 50, 50, 0.0f, 100, 100, 0, 1, 0, 0, 0);
	sprite->Render(dc, 100, 100, 1.0f, 100, 100, 0, 0, 1, 0, 0);
	sprite->Render(dc, 150, 150, 0.5f, 100, 100, 0, 1, 1, 0, 0);

	// 深度書き込みあり＆深度テストあり
	dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
	sprite->Render(dc, 350, 50, 0.0f, 100, 100, 0, 1, 0, 0, 0);
	sprite->Render(dc, 400, 100, 1.0f, 100, 100, 0, 0, 1, 0, 0);
	sprite->Render(dc, 450, 150, 0.5f, 100, 100, 0, 1, 1, 0, 0);

	// 深度書き込みのみ
	dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::WriteOnly), 0);
	sprite->Render(dc, 650, 50, 0.0f, 100, 100, 0, 1, 0, 0, 0);

	// 深度テストのみ
	dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::TestOnly), 0);
	sprite->Render(dc, 700, 100, 0.5f, 100, 100, 0, 1, 1, 0, 0);

	// 深度書き込みのみ
	dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::WriteOnly), 0);
	sprite->Render(dc, 750, 150, 1.0f, 100, 100, 0, 0, 1, 0, 0);
}


// コンストラクタ
BlendTestScene::BlendTestScene()
{
	ID3D11Device* device = Graphics::Instance().GetDevice();
	sprite = std::make_unique<Sprite>(device);
}

// 描画処理
void BlendTestScene::Render(float elapsedTime)
{
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
	RenderState* renderState = Graphics::Instance().GetRenderState();
	ID3D11SamplerState* samplers[] =
	{
	renderState->GetSamplerState(SamplerState::PointClamp)
	};
	dc->PSSetSamplers(0, _countof(samplers), samplers);
	// 深度テストなし＆深度書き込みなし（後に描いたものが手前になる）
	dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::NoTestNoWrite), 0);
	FLOAT blendFactor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	UINT sampleMask = 0xFFFFFFFF;
	// 加算合成テスト
	dc->OMSetBlendState(renderState->GetBlendState(BlendState::Opaque), blendFactor, sampleMask);
	sprite->Render(dc, 50, 50, 0.0f, 150, 150, 0, 0, 0, 0, 1.0f);
	sprite->Render(dc, 50, 50, 0.0f, 100, 100, 0, 1, 0, 0, 1.0f);
	dc->OMSetBlendState(renderState->GetBlendState(BlendState::Additive), blendFactor, sampleMask);
	sprite->Render(dc, 100, 100, 0.0f, 100, 100, 0, 0, 1, 0, 1.0f);
	// 減算合成テスト
	dc->OMSetBlendState(renderState->GetBlendState(BlendState::Opaque), blendFactor, sampleMask);
	sprite->Render(dc, 250, 50, 0.0f, 150, 150, 0, 1, 1, 1, 1.0f);
	sprite->Render(dc, 250, 50, 0.0f, 100, 100, 0, 1, 1, 0, 1.0f);
	dc->OMSetBlendState(renderState->GetBlendState(BlendState::Subtraction), blendFactor, sampleMask);
	sprite->Render(dc, 300, 100, 0.0f, 100, 100, 0, 0, 1, 0, 1.0f);
	// 乗算合成テスト
	dc->OMSetBlendState(renderState->GetBlendState(BlendState::Opaque), blendFactor, sampleMask);
	sprite->Render(dc, 450, 50, 0.0f, 150, 150, 0, 1, 1, 1, 1.0f);
	sprite->Render(dc, 450, 50, 0.0f, 100, 100, 0, 1, 1, 0, 1.0f);
	dc->OMSetBlendState(renderState->GetBlendState(BlendState::Subtraction), blendFactor, sampleMask);
	sprite->Render(dc, 500, 100, 0.0f, 100, 100, 0, 0.5f, 0.5f, 0.5f, 1.0f);
	// 透明合成テスト
	dc->OMSetBlendState(renderState->GetBlendState(BlendState::Opaque), blendFactor, sampleMask);
	sprite->Render(dc, 650, 50, 0.0f, 150, 150, 0, 1, 1, 1, 0.5f);
	sprite->Render(dc, 650, 50, 0.0f, 100, 100, 0, 1, 0, 0, 0.5f);
	dc->OMSetBlendState(renderState->GetBlendState(BlendState::Transparency), blendFactor, sampleMask);
	sprite->Render(dc, 700, 100, 0.0f, 100, 100, 0, 0, 0, 1, 0.5f);
}


// コンストラクタ
RasterizeTestScene::RasterizeTestScene()
{
	ID3D11Device* device = Graphics::Instance().GetDevice();
	sprite = std::make_unique<Sprite>(device);
}

// 描画処理
void RasterizeTestScene::Render(float elapsedTime)
{
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
	RenderState* renderState = Graphics::Instance().GetRenderState();
	ID3D11SamplerState* samplers[] =
	{
	renderState->GetSamplerState(SamplerState::PointClamp)
	};
	dc->PSSetSamplers(0, _countof(samplers), samplers);
	// 深度テストなし＆深度書き込みなし（後に描いたものが手前になる）
	dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::NoTestNoWrite), 0);
	// ベタ塗り＆カリングなし
	dc->RSSetState(renderState->GetRasterizerState(RasterizerState::SolidCullNone));
	sprite->Render(dc, 50, 50, 0.0f, 100, 100, 0, 1, 0, 0, 1);
	sprite->Render(dc, 150, 200, 0.0f, -100, 100, 0, 1, 0, 0, 1);
	// ベタ塗り＆裏面カリング
	dc->RSSetState(renderState->GetRasterizerState(RasterizerState::SolidCullBack));
	sprite->Render(dc, 200, 50, 0.0f, 100, 100, 0, 1, 0, 0, 1);
	sprite->Render(dc, 300, 200, 0.0f, -100, 100, 0, 1, 0, 0, 1);
	// ワイヤーフレーム＆カリングなし
	dc->RSSetState(renderState->GetRasterizerState(RasterizerState::WireCullNone));
	sprite->Render(dc, 350, 50, 0.0f, 100, 100, 0, 1, 0, 0, 1);
	sprite->Render(dc, 450, 200, 0.0f, -100, 100, 0, 1, 0, 0, 1);
	// ワイヤーフレーム＆裏面カリング
	dc->RSSetState(renderState->GetRasterizerState(RasterizerState::WireCullBack));
	sprite->Render(dc, 500, 50, 0.0f, 100, 100, 0, 1, 0, 0, 1);
	sprite->Render(dc, 600, 200, 0.0f, -100, 100, 0, 1, 0, 0, 1);
}


// コンストラクタ
GizmosTestScene::GizmosTestScene()
{
	float screenWidth = Graphics::Instance().GetScreenWidth();
	float screenHeight = Graphics::Instance().GetScreenHeight();
	// カメラ設定
	camera.SetPerspectiveFov(
		DirectX::XMConvertToRadians(45), // 画角
		screenWidth / screenHeight, // 画面アスペクト比
		0.1f, // ニアクリップ
		1000.0f // ファークリップ
	);
	camera.SetLookAt(
		{ 0, 1, -5 }, // 視点
		{ 0, 0, 0 }, // 注視点
		{ 0, 1, 0 } // 上ベクトル
	);
}

// 描画処理
void GizmosTestScene::Render(float elapsedTime)
{
	Gizmos* gizmos = Graphics::Instance().GetGizmos();

	// 回転処理
	rotation += 0.5f * elapsedTime;

	// 箱描画
	gizmos->DrawBox(
		{ 0, 0, 0 }, // 位置
		{ 0, rotation, 0 }, // 回転
		{ 1, 1, 1 }, // サイズ
		{ 1, 1, 1, 1 }); // 色

	// 球描画
	gizmos->DrawSphere(
		{ 2, 0, 0 }, // 位置
		1.0f, // 半径
		{ 1, 0, 0, 1 }); // 色

	// 描画コンテキスト設定
	RenderContext rc;
	rc.camera = &camera;
	rc.deviceContext = Graphics::Instance().GetDeviceContext();
	rc.renderState = Graphics::Instance().GetRenderState();

	// 描画実行
	gizmos->Render(rc);
}


// コンストラクタ
ModelTestScene::ModelTestScene()
{
	ID3D11Device* device = Graphics::Instance().GetDevice();
	float screenWidth = Graphics::Instance().GetScreenWidth();
	float screenHeight = Graphics::Instance().GetScreenHeight();

	// カメラ設定
	camera.SetPerspectiveFov(
		DirectX::XMConvertToRadians(45), // 画角
		screenWidth / screenHeight, // 画面アスペクト比
		0.1f, // ニアクリップ
		1000.0f // ファークリップ
	);
	camera.SetLookAt(
		{ 5, 3, -5 }, // 視点
		{ 0, 0, 0 }, // 注視点
		{ 0, 1, 0 } // 上ベクトル
	);

	// モデル作成
	//model = std::make_unique<Model>(device, "Data/Model/Cube/cube.000.fbx");
	//model = std::make_unique<Model>(device, "Data/Model/Cube/cube.001.0.fbx");
	//model = std::make_unique<Model>(device, "Data/Model/Cube/cube.001.2.fbx");
	//model = std::make_unique<Model>(device, "Data/Model/Cube/cube.001.1.fbx");
	//model = std::make_unique<Model>(device, "Data/Model/Cube/cube.003.1.fbx");
	//model = std::make_unique<Model>(device, "Data/Model/Cube/cube.004.fbx");
	model = std::make_unique<Model>(device, "Data/Model/Plantune/plantune.fbx", 0.01f);
	model->PlayAnimation(0, true);

	cameraController.SyncCameraToController(camera);
}

// 描画処理
void ModelTestScene::Render(float elapsedTime)
{
	// カメラ更新処理
	cameraController.Update();
	cameraController.SyncControllerToCamera(camera);

	// ワールド行列計算
	DirectX::XMMATRIX S = DirectX::XMMatrixScaling(scale.x, scale.y, scale.z);
	DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(angle.x, angle.y, angle.z);
	DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(position.x, position.y, position.z);
	DirectX::XMFLOAT4X4 worldTransform;
	DirectX::XMStoreFloat4x4(&worldTransform, S * R * T);

	// アニメーション更新
	model->UpdateAnimation(elapsedTime);

	// トランスフォーム更新
	model->UpdateTransform(worldTransform);

	// 描画コンテキスト設定
	RenderContext rc;
	rc.camera = &camera;
	rc.deviceContext = Graphics::Instance().GetDeviceContext();
	rc.renderState = Graphics::Instance().GetRenderState();

	// 描画
	Shader* shader = Graphics::Instance().GetShader(ShaderId::Phong);
	shader->Begin(rc);
	shader->Draw(rc, model.get());
	shader->End(rc);

	// デバッグメニュー描画
	DrawSceneGUI();
	DrawPropertyGUI();
	DrawAnimationGUI();
}

// シーンGUI描画
void ModelTestScene::DrawSceneGUI()
{
	ImVec2 pos = ImGui::GetMainViewport()->GetWorkPos();
	ImGui::SetNextWindowPos(ImVec2(pos.x + 10, pos.y + 10), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(300, 300), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Scene", nullptr, ImGuiWindowFlags_None))
	{
		if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
		{
			// 位置
			ImGui::DragFloat3("Position", &position.x, 0.1f);
			// 回転
			DirectX::XMFLOAT3 a;
			a.x = DirectX::XMConvertToDegrees(angle.x);
			a.y = DirectX::XMConvertToDegrees(angle.y);
			a.z = DirectX::XMConvertToDegrees(angle.z);
			ImGui::DragFloat3("Angle", &a.x, 1.0f);
			angle.x = DirectX::XMConvertToRadians(a.x);
			angle.y = DirectX::XMConvertToRadians(a.y);
			angle.z = DirectX::XMConvertToRadians(a.z);
			// スケール
			ImGui::DragFloat3("Scale", &scale.x, 0.01f);
		}
		if (ImGui::CollapsingHeader("Hierarchy", ImGuiTreeNodeFlags_DefaultOpen))
		{
			// ノードツリーを再帰的に描画する関数
			std::function<void(Model::Node*)> drawNodeTree = [&](Model::Node* node)
				{
					// 矢印をクリック、またはノードをダブルクリックで階層を開く
					ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow
						| ImGuiTreeNodeFlags_OpenOnDoubleClick;
					// 子がいない場合は矢印をつけない
					size_t childCount = node->children.size();
					if (childCount == 0)
					{
						nodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
					}
					// 選択フラグ
					if (selectionNode == node)
					{
						nodeFlags |= ImGuiTreeNodeFlags_Selected;
					}
					// ツリーノードを表示
					bool opened = ImGui::TreeNodeEx(node, nodeFlags, node->name.c_str());
					// フォーカスされたノードを選択する
					if (ImGui::IsItemFocused())
					{
						selectionNode = node;
					}
					// 開かれている場合、子階層も同じ処理を行う
					if (opened && childCount > 0)
					{
						for (Model::Node* child : node->children)
						{
							drawNodeTree(child);
						}
						ImGui::TreePop();
					}
				};
			// 再帰的にノードを描画
			drawNodeTree(model->GetRootNode());
		}
		ImGui::End();
	}
}

// プロパティGUI描画
void ModelTestScene::DrawPropertyGUI()
{
	ImVec2 pos = ImGui::GetMainViewport()->GetWorkPos();
	ImGui::SetNextWindowPos(ImVec2(pos.x + 970, pos.y + 10), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(300, 300), ImGuiCond_FirstUseEver);
	ImGui::Begin("Property", nullptr, ImGuiWindowFlags_None);
	if (selectionNode != nullptr)
	{
		if (ImGui::CollapsingHeader("Node", ImGuiTreeNodeFlags_DefaultOpen))
		{
			// 位置
			ImGui::DragFloat3("Position", &selectionNode->position.x, 0.1f);
			// 回転
			DirectX::XMFLOAT3 angle;
			TransformUtils::QuaternionToRollPitchYaw(selectionNode->rotation, angle.x, angle.y, angle.z);
			angle.x = DirectX::XMConvertToDegrees(angle.x);
			angle.y = DirectX::XMConvertToDegrees(angle.y);
			angle.z = DirectX::XMConvertToDegrees(angle.z);
			if (ImGui::DragFloat3("Rotation", &angle.x, 1.0f))
			{
				angle.x = DirectX::XMConvertToRadians(angle.x);
				angle.y = DirectX::XMConvertToRadians(angle.y);
				angle.z = DirectX::XMConvertToRadians(angle.z);
				DirectX::XMVECTOR Rotation = DirectX::XMQuaternionRotationRollPitchYaw(
					angle.x, angle.y, angle.z);
				DirectX::XMStoreFloat4(&selectionNode->rotation, Rotation);
			}
			// スケール
			ImGui::DragFloat3("Scale", &selectionNode->scale.x, 0.01f);
		}
	}
	ImGui::End();
}

// アニメーションGUI描画
void ModelTestScene::DrawAnimationGUI()
{
	ImVec2 pos = ImGui::GetMainViewport()->GetWorkPos();
	ImGui::SetNextWindowPos(ImVec2(pos.x + 10, pos.y + 350), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(300, 300), ImGuiCond_FirstUseEver);
	ImGui::Begin("Animation", nullptr, ImGuiWindowFlags_None);
	ImGui::Checkbox("Loop", &animationLoop);
	ImGui::DragFloat("BlendSec", &animationBlendSeconds, 0.01f);
	int index = 0;
	for (const Model::Animation& animation : model->GetAnimations())
	{
		ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_Leaf;
		ImGui::TreeNodeEx(&animation, nodeFlags, animation.name.c_str());
		// ダブルクリックでアニメーション再生
		if (ImGui::IsItemClicked())
		{
			if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
			{
				model->PlayAnimation(index, animationLoop, animationBlendSeconds);
			}
		}
		ImGui::TreePop();
		index++;
	}
	ImGui::End();
}