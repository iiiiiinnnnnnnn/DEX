#include <fstream>
#include "Sprite.h"
#include "Misc.h"
#include "GpuResourceUtils.h"
#include <Graphics.h>
#include "Scene.h"

// コンストラクタ
Sprite::Sprite(ID3D11Device* device)
	: Sprite(device, std::string(""))
{
}

// コンストラクタ
Sprite::Sprite(ID3D11Device* device, std::string filename)
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
		// 入力レイアウト
		D3D11_INPUT_ELEMENT_DESC inputElementDesc[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		hr = GpuResourceUtils::LoadVertexShader(
			device,
			"Data/Shader/SpriteVS.cso",
			inputElementDesc,
			ARRAYSIZE(inputElementDesc),
			inputLayout.GetAddressOf(),
			vertexShader.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
	}
	// ピクセルシェーダー
	{
		hr = GpuResourceUtils::LoadPixelShader(
			device,
			"Data/Shader/SpritePS.cso",
			pixelShader.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
	}
	// テクスチャの生成
	if (!filename.empty())
	{
		// テクスチャファイル読み込み
		D3D11_TEXTURE2D_DESC desc;
		hr = GpuResourceUtils::LoadTexture(device, filename, shaderResourceView.GetAddressOf(), &desc);
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
		textureWidth = static_cast<float>(desc.Width);
		textureHeight = static_cast<float>(desc.Height);
	}
	else
	{
		// ダミーテクスチャ生成
		D3D11_TEXTURE2D_DESC desc;
		hr = GpuResourceUtils::CreateDummyTexture(device, 0xFFFFFFFF, shaderResourceView.GetAddressOf(),
			&desc);
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
		textureWidth = static_cast<float>(desc.Width);
		textureHeight = static_cast<float>(desc.Height);
	}
}

// 描画実行
void Sprite::Render(Scene* scene,
	Vector3 dxyz, // 左上位置 // 奥行
	Vector2 dwh, // 幅、高さ
	Vector2 sxy, // 画像切り抜き位置
	Vector2 swh, // 画像切り抜きサイズ
	float angle, // 角度
	Color rgba = { 1, 1, 1, 1 }) const
{
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();

	/* 画面に描画するときは -1.0～1.0の範囲内で指定する必要がある。この空間を NDC 空間と呼ぶ */

	// 頂点座標
	Vector2 positions[] = {
		Vector2(dxyz.x, dxyz.y), // 左上
		Vector2(dxyz.x + dwh.x, dxyz.y), // 右上
		Vector2(dxyz.x, dxyz.y + dwh.y), // 左下
		Vector2(dxyz.x + dwh.x, dxyz.y + dwh.y), // 右下
	};

	// テクスチャ座標
	/* テクスチャ座標は0.0～1.0 の間で表現する */
	/* ピクセル単位で切り抜く画像の領域を指定 */
	Vector2 texcoords[] = {
		Vector2(sxy.x, sxy.y), // 左上
		Vector2(sxy.x + swh.x, sxy.y), // 右上
		Vector2(sxy.x, sxy.y + swh.y), // 左下
		Vector2(sxy.x + swh.x, sxy.y + swh.y), // 右下
	};

	// スプライトの中心を軸に回転
	{
		// スプライトの中心で回転させるために４頂点の中心位置が
		// 原点(0, 0)になるように一旦頂点を移動させる。
		float mx = dxyz.x + dwh.x * 0.5f;
		float my = dxyz.y + dwh.y * 0.5f;
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
			Vector2 r = p;
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
	for (Vector2& p : positions)
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
		v[i].position.z = dxyz.z; // 深度値を設定
		v[i].color.x = rgba.x;
		v[i].color.y = rgba.y;
		v[i].color.z = rgba.z;
		v[i].color.w = rgba.w;
		v[i].texcoord.x = texcoords[i].x / textureWidth;
		v[i].texcoord.y = texcoords[i].y / textureHeight;
	}

	// 頂点バッファの内容の編集を終了する。
	dc->Unmap(vertexBuffer.Get(), 0);

	// レンダーステート設定
	auto rs = scene->GetRenderState();
	const float blendFactor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	dc->OMSetBlendState(rs->GetBlendState(BlendState::Opaque), blendFactor, 0xFFFFFFFF);
	dc->OMSetDepthStencilState(rs->GetDepthStencilState(DepthState::NoTestNoWrite), 0);
	dc->RSSetState(rs->GetRasterizerState(RasterizerState::SolidCullNone));

	// GPUに描画するためのデータを渡す
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	dc->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);
	dc->IASetInputLayout(inputLayout.Get());
	dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	dc->VSSetShader(vertexShader.Get(), nullptr, 0);
	dc->PSSetShader(pixelShader.Get(), nullptr, 0);
	dc->PSSetShaderResources(0, 1, shaderResourceView.GetAddressOf()); // テクスチャを設定

	// 描画
	dc->Draw(4, 0);
}

void Sprite::Render(Scene* scene, Vector3 dxyz, Vector2 dwh, float angle, Color rgba) const
{
	Render(scene, dxyz, dwh, { 0, 0 }, { textureWidth, textureHeight }, angle, rgba);
}
