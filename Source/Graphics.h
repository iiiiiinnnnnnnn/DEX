#pragma once

#include <Common.h>

#include "stb_truetype.h"

#include "FrameBuffer.h"
#include "RenderState.h"

enum class ShaderId
{
	Phong,

	EnumCount
};

enum class FrameBufferId
{
	Display,
	Scene,
	Luminance,

	EnumCount
};

// グラフィックス
class Graphics
{
private:
	Graphics() = default;
	~Graphics() = default;
public:
	// インスタンス取得
	static Graphics& Instance()
	{
		// 扱いやすいようにシングルトンで取得できるようにする。
		static Graphics instance;
		return instance;
	}

	// 初期化
	void Initialize(HWND hWnd);

	// 画面表示
	void Present(UINT syncInterval);

	// デバイス取得
	ID3D11Device* GetDevice() { return device.Get(); }

	// デバイスコンテキスト取得
	ID3D11DeviceContext* GetDeviceContext() { return immediateContext.Get(); }

	// スクリーン幅取得
	float GetScreenWidth() const { return screenWidth; }

	// スクリーン高さ取得
	float GetScreenHeight() const { return screenHeight; }

	// フレームバッファ取得
	FrameBuffer* GetFrameBuffer(FrameBufferId frameBufferId)
	{
		return frameBuffers[static_cast<int>(frameBufferId)].get();
	}

	// レンダーステート取得
	RenderState* GetRenderState() { return renderState.get(); }

	// テキスト描画
	void Text(int fontIndex, const std::u32string& text, float x, float y, float size, Color color);

private:
	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> immediateContext;
	Microsoft::WRL::ComPtr<IDXGISwapChain> swapchain;

	std::unique_ptr<FrameBuffer> frameBuffers[static_cast<int>(FrameBufferId::EnumCount)];
	std::unique_ptr<RenderState> renderState;

	float screenWidth;
	float screenHeight;

	struct FontGlyph
	{
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
		int width;
		int height;
	};

	struct FontData
	{
		stbtt_fontinfo font;
		std::vector<unsigned char> ttfBuffer;

		std::unordered_map<int, FontGlyph> glyphCache;
	};

	std::vector<FontData> fonts;
	std::unique_ptr<DirectX::SpriteBatch> spriteBatch;
	std::unique_ptr<DirectX::CommonStates> commonStates;

	// フォントの読み込み
	void LoadAllFonts();

	// グリフ取得
	FontGlyph& GetGlyph(int fontIndex, int codepoint, float size);
};