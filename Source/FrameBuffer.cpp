#include "Misc.h"
#include "FrameBuffer.h"

// コンストラクタ
FrameBuffer::FrameBuffer(ID3D11Device* device, IDXGISwapChain* swapchain)
{
	HRESULT hr = S_OK;
	UINT width, height;
	// レンダーターゲットビューの生成
	{
		// スワップチェーンからバックバッファテクスチャを取得する。
		// ※スワップチェーンに内包されているバックバッファテクスチャは'色'を書き込むテクスチャ。
		Microsoft::WRL::ComPtr<ID3D11Texture2D> texture2d;
		hr = swapchain->GetBuffer(
			0,
			__uuidof(ID3D11Texture2D),
			reinterpret_cast<void**>(texture2d.GetAddressOf()));
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
		// バックバッファテクスチャへの書き込みの窓口となるレンダーターゲットビューを生成する。
		// テクスチャに直接書き込みはできないのでビューと呼ばれる窓口を通して描く必要がある
		hr = device->CreateRenderTargetView(texture2d.Get(), nullptr, renderTargetView.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
		// バックバッファテクスチャからサイズ情報を取得1
		D3D11_TEXTURE2D_DESC texture2dDesc;
		texture2d->GetDesc(&texture2dDesc);
		width = texture2dDesc.Width;
		height = texture2dDesc.Height;
	}
	// ビューポート
	{
		viewport.Width = static_cast<float>(width);
		viewport.Height = static_cast<float>(height);
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		viewport.TopLeftX = 0.0f;
		viewport.TopLeftY = 0.0f;
	}
}

// クリア
void FrameBuffer::Clear(ID3D11DeviceContext* dc, float r, float g, float b, float a)
{
	float color[4]{ r, g, b, a };
	// レンダーターゲットビューを通してバックバッファの色をクリアする
	dc->ClearRenderTargetView(renderTargetView.Get(), color);
}

// レンダーターゲット設定
void FrameBuffer::SetRenderTargets(ID3D11DeviceContext* dc)
{
	dc->RSSetViewports(1, &viewport);
	// レンダーターゲットビューを通してバックバッファに CG を描く
	dc->OMSetRenderTargets(1, renderTargetView.GetAddressOf(), nullptr);
}
