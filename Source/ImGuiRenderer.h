#pragma once

#include "Common.h"

class ImGuiRenderer
{
public:
	// 初期化
	static void Initialize(HWND hWnd, ID3D11Device* device, ID3D11DeviceContext* dc);
	
	// 終了化
	static void Finalize();

	// フレーム開始処理
	static void NewFrame();

	// 描画実行
	static void Render(ID3D11DeviceContext* context);

	// WIN32メッセージハンドラー
	static LRESULT HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	// フォントSRV取得
	static ID3D11ShaderResourceView* GetFontSRV();

	static ImFont* GetBillboardFont();

private:
	static ID3D11ShaderResourceView* fontSRV;
	static ImFont* billboardFont;
};
