#include <fstream>
#include "Sprite.h"
#include "Misc.h"

// コンストラクタ
Sprite::Sprite(ID3D11Device* device)
{
	HRESULT hr = S_OK;

	// 頂点バッファの生成
	{
		// 頂点バッファを作成するための設定オプション
		/* 4 頂点分の頂点バッファを生成。D3D11_USAGE_DYNAMIC,D3D11_CPU_ACCESS_WRITEを指定することで毎フレーム頂点を編集できるようになる。 */
		D3D11_BUFFER_DESC buffer_desc = {};
		buffer_desc.ByteWidth = sizeof(Vertex) * 4;
		buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
		buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		buffer_desc.MiscFlags = 0;
		buffer_desc.StructureByteStride = 0;

		// 頂点バッファオブジェクトの生成
		hr = device->CreateBuffer(&buffer_desc, nullptr, vertexBuffer.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
	}

	// 頂点シェーダー
	{
		std::ifstream fin("Data/Shader/SpriteVS.cso", std::ios::in | std::ios::binary);

		// ファイルサイズを求める
		fin.seekg(0, std::ios_base::end);
		long long size = fin.tellg();
		fin.seekg(0, std::ios_base::beg);

		// 読み込み
		std::unique_ptr<char[]> data = std::make_unique<char[]>(size);
		fin.read(data.get(), size);

		// 頂点シェーダー生成
		HRESULT hr = device->CreateVertexShader(data.get(), size, nullptr, vertexShader.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));

		// 入力レイアウト
		D3D11_INPUT_ELEMENT_DESC inputElementDesc[] =
		{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT,
		D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT,
		D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		hr = device->CreateInputLayout(inputElementDesc, ARRAYSIZE(inputElementDesc),
			data.get(), size, inputLayout.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
	}
	// ピクセルシェーダー
	{
		std::ifstream fin("Data/Shader/SpritePS.cso",
			std::ios::in | std::ios::binary);

		// ファイルサイズを求める
		fin.seekg(0, std::ios_base::end);
		long long size = fin.tellg();
		fin.seekg(0, std::ios_base::beg);

		// 読み込み
		std::unique_ptr<char[]> data = std::make_unique<char[]>(size);
		fin.read(data.get(), size);

		// ピクセルシェーダー生成
		HRESULT hr = device->CreatePixelShader(data.get(), size, nullptr, pixelShader.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
	}
}

// 描画実行
void Sprite::Render(ID3D11DeviceContext* dc,
	float dx, float dy, // 左上位置
	float dw, float dh, // 幅、高さ
	float angle, // 角度
	float r, float g, float b, float a // 色
) const
{
	/* 画面に描画するときは -1.0～1.0の範囲内で指定する必要がある。この空間を NDC 空間と呼ぶ */

	// 頂点座標
	DirectX::XMFLOAT2 positions[] = {
		DirectX::XMFLOAT2(dx, dy), // 左上
		DirectX::XMFLOAT2(dx + dw, dy), // 右上
		DirectX::XMFLOAT2(dx, dy + dh), // 左下
		DirectX::XMFLOAT2(dx + dw, dy + dh), // 右下
	};

	// スプライトの中心を軸に回転
	{
		// スプライトの中心で回転させるために４頂点の中心位置が
		// 原点(0, 0)になるように一旦頂点を移動させる。
		float mx = dx + dw * 0.5f;
		float my = dy + dh * 0.5f;
		for (auto& p : positions)
		{
			p.x -= mx;
			p.y -= my;
		}

		// 頂点を回転させる
		float theta = DirectX::XMConvertToRadians(angle);
		float c = cosf(theta);
		float s = sinf(theta);
		for (auto& p : positions)
		{
			DirectX::XMFLOAT2 r = p;
			p.x = c * r.x + -s * r.y;
			p.y = s * r.x + c * r.y;
		}

		// 回転のために移動させた頂点を元の位置に戻す
		for (auto& p : positions)
		{
			p.x += mx;
			p.y += my;
		}
	}

	// 現在設定されているビューポートからスクリーンサイズを取得する。
	D3D11_VIEWPORT viewport;
	UINT numViewports = 1;
	dc->RSGetViewports(&numViewports, &viewport);
	float screenWidth = viewport.Width;
	float screenHeight = viewport.Height;
	// スクリーン座標系からNDC座標系へ変換する。
	for (DirectX::XMFLOAT2& p : positions)
	{
		p.x = 2.0f * p.x / screenWidth - 1.0f;
		p.y = 1.0f - 2.0f * p.y / screenHeight;
	}

	// 頂点バッファの内容の編集を開始する。
	D3D11_MAPPED_SUBRESOURCE mappedSubresource;
	HRESULT hr = dc->Map(vertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresource);
	_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));

	// 頂点バッファの内容を編集
	Vertex* v = static_cast<Vertex*>(mappedSubresource.pData);
	for (int i = 0; i < 4; ++i)
	{
		v[i].position.x = positions[i].x;
		v[i].position.y = positions[i].y;
		v[i].position.z = 0.0f;
		v[i].color.x = r;
		v[i].color.y = g;
		v[i].color.z = b;
		v[i].color.w = a;
	}

	// 頂点バッファの内容の編集を終了する。
	dc->Unmap(vertexBuffer.Get(), 0);

	// GPUに描画するためのデータを渡す
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	dc->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);
	dc->IASetInputLayout(inputLayout.Get());
	dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	dc->VSSetShader(vertexShader.Get(), nullptr, 0);
	dc->PSSetShader(pixelShader.Get(), nullptr, 0);

	// 描画
	dc->Draw(4, 0);
}