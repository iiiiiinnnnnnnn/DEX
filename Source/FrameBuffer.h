#pragma once

#include <wrl.h>
#include <d3d11.h>

class FrameBuffer
{
public:
	FrameBuffer(ID3D11Device* device, UINT width, UINT height);
	FrameBuffer(ID3D11Device* device, IDXGISwapChain* swapchain);

	// カラーマップ取得
	ID3D11ShaderResourceView* GetColorMap() const { return colorMap.Get(); }

	// クリア
	void Clear(ID3D11DeviceContext* dc, float r, float g, float b, float a);

	// レンダーターゲット設定
	void SetRenderTargets(ID3D11DeviceContext* dc);

private:
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderTargetView;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthStencilView;
	D3D11_VIEWPORT viewport;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> colorMap;
};