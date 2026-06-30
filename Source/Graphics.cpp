#include "Misc.h"
#include "Graphics.h"

// 初期化
void Graphics::Initialize(HWND hWnd)
{
	// 画面のサイズを取得する。
	RECT rc;
	GetClientRect(hWnd, &rc);
	UINT screenWidth = rc.right - rc.left;
	UINT screenHeight = rc.bottom - rc.top;
	this->screenWidth = static_cast<float>(screenWidth);
	this->screenHeight = static_cast<float>(screenHeight);
	HRESULT hr = S_OK;

	// デバイス＆スワップチェーンの生成
	{
		UINT createDeviceFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
		createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
		D3D_FEATURE_LEVEL featureLevels[] =
		{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1,
		};
		// スワップチェーンを作成するための設定オプション
		DXGI_SWAP_CHAIN_DESC swapchainDesc;
		{
			swapchainDesc.BufferDesc.Width = screenWidth;
			swapchainDesc.BufferDesc.Height = screenHeight;
			swapchainDesc.BufferDesc.RefreshRate.Numerator = 60;
			swapchainDesc.BufferDesc.RefreshRate.Denominator = 1;
			swapchainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			swapchainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
			swapchainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
			swapchainDesc.SampleDesc.Count = 1;
			swapchainDesc.SampleDesc.Quality = 0;
			swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			swapchainDesc.BufferCount = 1;
			swapchainDesc.OutputWindow = hWnd;
			swapchainDesc.Windowed = TRUE;
			swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
			swapchainDesc.Flags = 0;
		}
		D3D_FEATURE_LEVEL featureLevel;
		// デバイス＆スワップチェーンの生成
		hr = D3D11CreateDeviceAndSwapChain(
			nullptr,
			D3D_DRIVER_TYPE_HARDWARE,
			nullptr,
			createDeviceFlags,
			featureLevels,
			ARRAYSIZE(featureLevels),
			D3D11_SDK_VERSION,
			&swapchainDesc,
			swapchain.GetAddressOf(),
			device.GetAddressOf(),
			&featureLevel,
			immediateContext.GetAddressOf()
		);
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
	}

	// フレームバッファ作成
	frameBuffers[static_cast<int>(FrameBufferId::Display)] = std::make_unique<FrameBuffer>(device.Get(), swapchain.Get());
	frameBuffers[static_cast<int>(FrameBufferId::Scene)] = std::make_unique<FrameBuffer>(device.Get(), screenWidth, screenHeight);
	frameBuffers[static_cast<int>(FrameBufferId::Luminance)] = std::make_unique<FrameBuffer>(device.Get(), screenWidth, screenHeight);

	// レンダーステート生成
	renderState = std::make_unique<RenderState>(device.Get());

	// SpriteBatch生成
	spriteBatch = std::make_unique<DirectX::SpriteBatch>(immediateContext.Get());

	// ステート生成（ブレンドとか）
	commonStates = std::make_unique<DirectX::CommonStates>(device.Get());

	// フォントの読み込み
	LoadAllFonts();
}

// フォントの読み込み
void Graphics::LoadAllFonts()
{
	std::filesystem::path fontPath = "Data/Font";

	std::ifstream file(fontPath / "fonts.txt");
	std::string line;

	while (std::getline(file, line)) {
		if (!line.empty())
		{
			// フォント登録
			if (std::filesystem::exists(fontPath / line))
			{
				FontData fontData;

				std::ifstream ttf(fontPath / line, std::ios::binary);
				fontData.ttfBuffer = std::vector<unsigned char>(
					std::istreambuf_iterator<char>(ttf),
					std::istreambuf_iterator<char>());

				stbtt_InitFont(&fontData.font, fontData.ttfBuffer.data(), 0);

				fonts.emplace_back(std::move(fontData));
			}
			else {
				_ASSERT_EXPR(false, ("Font file not found: " + line).c_str());
			}
		}
	}
}

// グリフ取得
Graphics::FontGlyph& Graphics::GetGlyph(int fontIndex, int codepoint, float size)
{
	FontData& font = fonts[fontIndex];

	if (font.glyphCache.count(codepoint))
		return font.glyphCache[codepoint];

	float scale = stbtt_ScaleForPixelHeight(&font.font, size);

	int w, h, xoff, yoff;
	unsigned char* bitmap = stbtt_GetCodepointBitmap(
		&font.font,
		0,
		scale,
		codepoint,
		&w,
		&h,
		&xoff,
		&yoff
	);

	// R8 → RGBA8 に変換（簡易版）
	std::vector<uint32_t> rgba(w * h);
	for (int i = 0; i < w * h; ++i) {
		uint8_t v = bitmap[i];
		rgba[i] = (v << 24) | (v << 16) | (v << 8) | v; // A,R,G,B 順
	}

	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = w;
	desc.Height = h;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA sub = {};
	sub.pSysMem = rgba.data();
	sub.SysMemPitch = w * 4;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
	device->CreateTexture2D(&desc, &sub, &tex);

	FontGlyph glyph;
	glyph.width = w;
	glyph.height = h;

	device->CreateShaderResourceView(tex.Get(), nullptr, &glyph.srv);

	stbtt_FreeBitmap(bitmap, nullptr);

	font.glyphCache[codepoint] = glyph;

	return font.glyphCache[codepoint];
}

void Graphics::Text(
	int fontIndex,
	const std::u32string& text,
	float x,
	float y,
	float size,
	Color color)
{
	float penX = x;

	spriteBatch->Begin(
		DirectX::SpriteSortMode_Deferred,
		commonStates->AlphaBlend(),
		nullptr,
		commonStates->DepthNone(),
		nullptr
	);

	for (char32_t c : text)
	{
		FontGlyph& glyph = GetGlyph(fontIndex, (int)c, size);

		spriteBatch->Draw(
			glyph.srv.Get(),
			DirectX::XMFLOAT2(penX, y),
			nullptr,
			color,
			0.0f,
			DirectX::XMFLOAT2(0, 0),
			1.0f
		);

		penX += glyph.width;
	}

	spriteBatch->End();
}

// 画面表示
void Graphics::Present(UINT syncInterval)
{
	swapchain->Present(syncInterval, 0); // バックバッファの内容を画面に表示する
}