#include "Scene.h"
#include "Graphics.h"

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
	model = std::make_unique<Model>(device, "Data/Model/Cube/cube.000.fbx");
}

// 描画処理
void ModelTestScene::Render(float elapsedTime)
{
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
}