#pragma once

#include <d3d11.h>
#include <wrl.h>
#include <memory>

#include "FrameBuffer.h"
#include "RenderState.h"
#include "Gizmos.h"
#include "Shader.h"
#include "ShadowMap.h"

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

	// ギズモ取得
	Gizmos* GetGizmos() { return gizmos.get(); }

	// シェーダー取得
	Shader* GetShader(ShaderId shaderId) { return shaders[static_cast<int>(shaderId)].get(); }

	// シャドウマップ取得
	ShadowMap* GetShadowMap() { return shadowMap.get(); }

private:
	// ComPtr で DirectX のオブジェクトをスマートポインタとして扱う。
	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> immediateContext;
	Microsoft::WRL::ComPtr<IDXGISwapChain> swapchain;

	std::unique_ptr<FrameBuffer> frameBuffers[static_cast<int>(FrameBufferId::EnumCount)];
	std::unique_ptr<RenderState> renderState;
	std::unique_ptr<Gizmos> gizmos;
	std::unique_ptr<Shader> shaders[static_cast<int>(ShaderId::EnumCount)];
	std::unique_ptr<ShadowMap> shadowMap;

	float screenWidth;
	float screenHeight;
};