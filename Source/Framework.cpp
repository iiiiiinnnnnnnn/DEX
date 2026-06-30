#include "Framework.h"

#include <imgui.h>
#include "ImGuiRenderer.h"

#include "Graphics.h"
#include <Editor.h>
#include <AssetManager.h>

// 垂直同期間隔設定
static const int syncInterval = 1;

// コンストラクタ
Framework::Framework(HWND hWnd)
	: hWnd(hWnd)
{
	// グラフィックス初期化
	Graphics::Instance().Initialize(hWnd);

	// IMGUI初期化
	ImGuiRenderer::Initialize(hWnd, Graphics::Instance().GetDevice(), Graphics::Instance().GetDeviceContext());

	// アセットマネージャー
	AssetManager::Instance().Initialize();

	// シーン初期化
	scene = std::make_unique<Scene>();
	scene->Load("empty.scene");

	// エディタ初期化
	Editor::Instance().Initialize(scene.get());
}

// デストラクタ
Framework::~Framework()
{
	// IMGUI終了化
	ImGuiRenderer::Finalize();

	scene->Finalize();
}

// 更新処理
void Framework::Update(float elapsedTime)
{
	// プレイ中はシーン更新処理
	if (Editor::Instance().IsPlaying()) {
		scene->Update(elapsedTime);
	}
}

// 描画処理
void Framework::Render(float elapsedTime)
{
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();

	// IMGUIフレーム開始処理
	ImGuiRenderer::NewFrame();

	// 画面クリア
	Graphics::Instance().GetFrameBuffer(FrameBufferId::Display)->Clear(dc, 0, 0, 1, 1);

	// レンダーターゲット設定
	Graphics::Instance().GetFrameBuffer(FrameBufferId::Display)->SetRenderTargets(dc);

	// シーン描画処理
	scene->Render(elapsedTime);

	// エディタ描画処理
	Editor::Instance().Render(scene.get(), fps);

#if 0
	// IMGUIデモウインドウ描画（IMGUI機能テスト用）
	ImGui::ShowDemoWindow();
#endif

	// IMGUI描画
	ImGuiRenderer::Render(dc);

	// 画面表示
	/* 画面を表示する際にフレームの同期をとる
		0:可変フレームレート
		1 : 60FPS
		2 : 30FPS
		3 : 20FPS
		4 : 15FPS */
	Graphics::Instance().Present(syncInterval);
}

// フレームレート計算
void Framework::CalculateFrameStats()
{
	// Code computes the average frames per second, and also the 
	// average time it takes to render one frame.  These stats 
	// are appended to the window caption bar.
	static int frames = 0;
	static float time_tlapsed = 0.0f;

	frames++;

	// Compute averages over one second period.
	if ((timer.TimeStamp() - time_tlapsed) >= 1.0f)
	{
		fps = static_cast<float>(frames); // fps = frameCnt / 1
		float mspf = 1000.0f / fps;

		// Reset for next average.
		frames = 0;
		time_tlapsed += 1.0f;
	}
}

// アプリケーションループ
int Framework::Run()
{
	MSG msg = {};

	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			timer.Tick();
			CalculateFrameStats();

			/*float elapsedTime = syncInterval == 0
				? timer.TimeInterval()
				: syncInterval / 60.0f
				;*/
			//経過時間を取得(PC設定に左右されないはず)
			float elapsedTime = timer.TimeInterval();

			Update(elapsedTime);
			Render(elapsedTime);
		}
	}
	return static_cast<int>(msg.wParam);
}

// メッセージハンドラ
LRESULT CALLBACK Framework::HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGuiRenderer::HandleMessage(hWnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc;
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;
	}
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_CREATE:
		break;
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE) PostMessage(hWnd, WM_CLOSE, 0, 0);
		break;
	case WM_ENTERSIZEMOVE:
		// WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
		timer.Stop();
		break;
	case WM_EXITSIZEMOVE:
		// WM_EXITSIZEMOVE is sent when the user releases the resize bars.
		// Here we reset everything based on the new window dimensions.
		timer.Start();
		break;
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}
