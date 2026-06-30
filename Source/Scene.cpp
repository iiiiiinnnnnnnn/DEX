#include "Scene.h"
#include "Graphics.h"

// コンストラクタ
void Scene::Initialize()
{
	// テストシーン：リミナル
#if 0
	auto lights = AddGameObject("Lights");
	{
		// --- Directional Light ---
		lights->AddGameObject("Directional Light")
			->AddComponent<DirectionalLight>(Color(1, 1, 1), 0)
			->owner->transform.rotation =
			Quaternion::CreateFromYawPitchRoll(0, -90 * DEG2RAD, 0);
		// 白
		lights->AddGameObject("Point Light1")
			->AddComponent<PointLight>(Color(1, 0.7f, 0), 28.0f, 1.0f)
			->owner->transform.position = { 0, 2, -6.5f };

		// 白
		lights->AddGameObject("Point Light2")
			->AddComponent<PointLight>(Color(1, 0.7f, 0), 28.0f, 1.0f)
			->owner->transform.position = { 0, 2, 6.5f };
	}

	// モデル配置
	AddGameObject("liminal")
		->AddComponent<Model>("Data/Model/liminal/liminal.mdl");

	auto plantune = AddGameObject("nico")
		->AddComponent<Model>("Data/Model/nico/nico.mdl");

	plantune->owner->transform.position = { 0, 0.5f ,0 };
	plantune->owner->transform.scale = { 2 ,2 ,2 };
	plantune->AppendAnimation("Data/Model/nico/NIC_Idle.anim");
	plantune->PlayAnimation(0, true);

	// カメラ1
	auto camera1 = AddGameObject("Camera1", Vector3(0, 2, 5))->AddComponent<Camera>();

	camera1->SetPerspectiveFov(
		45 * DEG2RAD, // 画角
		screenWidth / screenHeight, // 画面アスペクト比
		0.1f, // ニアクリップ
		1000.0f // ファークリップ
	);
	camera1->SetLookAt(
		{ 0, 10, 20 }, // 視点
		{ 0, 0, 0 }, // 注視点
		{ 0, 1, 0 } // 上ベクトル
	);
	//cameraController.SyncCameraToController(camera);

	// カメラ2
	AddGameObject("Camera2")->AddComponent<Camera>();
#endif
}

void Scene::Finalize()
{
	rootObjects.clear();  // 既存のオブジェクトを削除
	postEffect.reset();
	gizmosRenderer.reset();
	shadowMapRenderer.reset();
	skyMapRenderer.reset();
	mainCamera = nullptr;
}

void Scene::Update(float elapsedTime)
{
	UpdateAllObjects(elapsedTime);
}

// 描画処理
void Scene::Render(float elapsedTime)
{
	auto& graphics = Graphics::Instance();
	auto dc = graphics.GetDeviceContext();

	// レンダラー取得
	RenderState* rs = graphics.GetRenderState();

	// 描画コンテキスト設定
	renderState = rs;

	// カメラがないと描画できない
	if (!CheckMainCamera())
	{
		graphics.Text(0, U"カメラが見つかりません",
			graphics.GetScreenWidth() / 2 - 100,
			graphics.GetScreenHeight() / 2 - 30,
			30,
			{ 1,1,1,0.5f });

		return;
	}

	// フレームバッファを取得
	FrameBuffer* displayFB = graphics.GetFrameBuffer(FrameBufferId::Display);
	FrameBuffer* sceneFB = graphics.GetFrameBuffer(FrameBufferId::Scene);
	FrameBuffer* luminanceFB = graphics.GetFrameBuffer(FrameBufferId::Luminance);

	// シーン用のフレームバッファにスプライトを描画
	sceneFB->SetRenderTargets(dc);
	sceneFB->Clear(dc, 0.3f, 0.3f, 0.3f, 1.0f);

	// カメラ更新処理
	{
		//cameraController.Update();
		//cameraController.SyncControllerToCamera(camera);
	}

	skyMapRenderer->Render(this);

	CollectAndSetLights();
	RenderAllObjects();
	RenderAllShadows();

	// スプライト描画
	{
		/*if (sprites[0])
		{
			ID3D11SamplerState* samplers[] =
			{
				renderState->GetSamplerState(SamplerState::PointClamp)
			};
			dc->PSSetSamplers(0, _countof(samplers), samplers);

			sprites[0]->Render(rc, { 100, 50, 0 }, { 100, 50 });
		}*/
	}

	// ポストエフェクト
	{
		// ポストエフェクト処理開始
		postEffect->Begin(this);

		// 輝度抽出処理
		//displayFB->SetRenderTargets(dc); // バックバッファに輝度を抽出した結果を描画
		luminanceFB->SetRenderTargets(dc); //輝度抽出用のフレームバッファに描画
		postEffect->LuminanceExtraction(this, sceneFB->GetColorMap());

		// ブルーム処理
		displayFB->SetRenderTargets(dc); // バックバッファにブルーム処理した結果を描画
		postEffect->Bloom(this, sceneFB->GetColorMap(), luminanceFB->GetColorMap());

		// 終了処理
		postEffect->End(this);
	}

	gizmosRenderer->Render(this);
}

void Scene::Save(std::filesystem::path path)
{
	json j;

	skyMapRenderer->Serialize(j["skyMap"]);
	shadowMapRenderer->Serialize(j["shadowMap"]);
	postEffect->Serialize(j["postEffect"]);

	json& objects = j["root_objects"];
	objects = json::array();

	for (auto& root : this->rootObjects)
	{
		json obj;
		SaveGameObject(root.get(), obj);
		objects.push_back(obj);
	}

	std::ofstream file(path);
	if (!file.is_open())
	{
		return _ASSERT_EXPR(false, "jsonファイル書き込みエラー");
	}

	file << j.dump(2);  // インデント2で読みやすく
	file.close();
}

void Scene::Load(std::filesystem::path path)
{
	Finalize();  // 現在のシーンをクリア

	// ポストエフェクト生成
	postEffect = std::make_unique<PostEffect>(Graphics::Instance().GetDevice());

	// ギズムレンダラー生成
	gizmosRenderer = std::make_unique<GizmosRenderer>();

	// シャドウマップレンダラー生成
	shadowMapRenderer = std::make_unique<ShadowMapRenderer>(2048 * 6);

	// スカイマップ生成
	skyMapRenderer = std::make_unique<SkyMapRenderer>();
	skyMapRenderer->LoadPanorama("sky.dds");

	// ファイル読み込み

	std::ifstream file(path);
	if (!file.is_open())
	{
		return _ASSERT_EXPR(false, "jsonファイル読み込みエラー");
	}

	json j;
	try
	{
		file >> j;
	}
	catch (...)
	{
		return _ASSERT_EXPR(false, "jsonファイルパースエラー");
	}

	if (!j.contains("root_objects"))
		return _ASSERT_EXPR(false, "ルートオブジェクトがありません");

	for (const auto& obj_json : j["root_objects"])
	{
		LoadGameObject(obj_json, nullptr);  // ルートオブジェクトとして追加
	}

	// その他の情報復元

	if(!j.contains("postEffect"))
		return _ASSERT_EXPR(false, "postEffect情報がありません");
	else
		postEffect->Deserialize(j["postEffect"]);

	if (!j.contains("shadowMap"))
		return _ASSERT_EXPR(false, "shadowMap情報がありません");
	else
		shadowMapRenderer->Deserialize(j["shadowMap"]);

	if(!j.contains("skyMap"))
		return _ASSERT_EXPR(false, "skyMap情報がありません");
	else
		skyMapRenderer->Deserialize(j["skyMap"]);

	Initialize(); // 初期化処理(いらんかも)

	CheckMainCamera();
}
