#include "ImGuiRenderer.h"

#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include <ImGuizmo.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
ID3D11ShaderResourceView* ImGuiRenderer::fontSRV = nullptr;
ImFont* ImGuiRenderer::billboardFont = nullptr;

// 初期化
void ImGuiRenderer::Initialize(HWND hWnd, ID3D11Device* device, ID3D11DeviceContext* dc)
{
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();

	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	//io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
	//io.ConfigViewportsNoAutoMerge = true;
	//io.ConfigViewportsNoTaskBarIcon = true;
	//io.ConfigViewportsNoDefaultParent = true;
	//io.ConfigDockingAlwaysTabBar = true;
	//io.ConfigDockingTransparentPayload = true;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(hWnd);
	ImGui_ImplDX11_Init(device, dc);

	// 日本語フォントを追加
	ImFont* baseFont = io.Fonts->AddFontFromFileTTF(
		"C:/Windows/Fonts/meiryo.ttc",
		20.0f,
		nullptr,
		io.Fonts->GetGlyphRangesJapanese()
	);
	IM_ASSERT(baseFont);

	// Font Awesomeをマージ
	static const ImWchar icon_ranges[] = { 0xf000, 0xf8ff, 0 };
	ImFontConfig config;
	config.MergeMode = true;
	config.PixelSnapH = true;
	io.Fonts->AddFontFromFileTTF(
		"fa-solid-900.ttf",
		18.0f,
		&config,
		icon_ranges
	);

	// Billboard用
	ImFontConfig iconConfig;
	iconConfig.MergeMode = false;
	billboardFont = io.Fonts->AddFontFromFileTTF(
		"fa-solid-900.ttf",
		80.0f,
		&iconConfig,
		icon_ranges
	);

	// フォントテクスチャ生成
	ImGui_ImplDX11_CreateDeviceObjects();
}

// 終了化
void ImGuiRenderer::Finalize()
{
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

// フレーム開始処理
void ImGuiRenderer::NewFrame()
{
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();

	ImGui::NewFrame();
	ImGuizmo::BeginFrame();

	ImGuiIO& io = ImGui::GetIO();
	if (!fontSRV)
	{
		fontSRV = (ID3D11ShaderResourceView*)io.Fonts->TexID.GetTexID();
	}
}

// 描画
void ImGuiRenderer::Render(ID3D11DeviceContext* context)
{
	// Rendering
	ImGui::Render();

	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	// Update and Render additional Platform Windows
	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}
}

// WIN32メッセージハンドラー
LRESULT ImGuiRenderer::HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
}

ID3D11ShaderResourceView* ImGuiRenderer::GetFontSRV()
{
	return fontSRV;
}

ImFont* ImGuiRenderer::GetBillboardFont()
{
	return billboardFont;
}
