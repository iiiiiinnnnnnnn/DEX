#include "Misc.h"
#include "GpuResourceUtils.h"
#include "Scene.h"
#include "SkyMapRenderer.h"
#include <Graphics.h>
#include <AssetManager.h>
#include "Camera.h"

// コンストラクタ
SkyMapRenderer::SkyMapRenderer()
{
	HRESULT hr = S_OK;
	auto device = Graphics::Instance().GetDevice();

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

	// スカイマップ用に深度値を書き込まない深度ステンシルステートの生成
	{
		D3D11_DEPTH_STENCIL_DESC depth_stencil_desc{};
		depth_stencil_desc.DepthEnable = TRUE;
		depth_stencil_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		depth_stencil_desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
		hr = device->CreateDepthStencilState(&depth_stencil_desc, depthStencilState.GetAddressOf());
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
			"SkyMapVS._cso",
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
			"SkyMapPS._cso",
			pixelShader.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
	}

	// スカイマップ用定数バッファ
	GpuResourceUtils::CreateConstantBuffer(
		device,
		sizeof(CbSkyMap),
		skyMapConstantBuffer.GetAddressOf());
}

void SkyMapRenderer::Render(Scene* scene)
{
	// BEGIN

	auto& graphics = Graphics::Instance();
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();

	// シェーダー設定
	dc->VSSetShader(vertexShader.Get(), nullptr, 0);
	dc->PSSetShader(pixelShader.Get(), nullptr, 0);
	dc->IASetInputLayout(inputLayout.Get());

	auto rs = scene->GetRenderState();
	ID3D11SamplerState* samplers[] = {
		rs->GetSamplerState(SamplerState::LinearClamp),
	};
	dc->PSSetSamplers(0, 1, samplers);

	// レンダーステート設定
	Color blendFactor = { 1.0f, 1.0f, 1.0f, 1.0f };
	dc->OMSetBlendState(rs->GetBlendState(BlendState::Opaque), blendFactor, 0xFFFFFFFF);
	dc->OMSetDepthStencilState(depthStencilState.Get(), 0);
	dc->RSSetState(rs->GetRasterizerState(RasterizerState::SolidCullNone));

	// 定数バッファ設定
	ID3D11Buffer* constantBuffers[] =
	{
		skyMapConstantBuffer.Get(),
	};
	dc->VSSetConstantBuffers(0, _countof(constantBuffers), constantBuffers);
	dc->PSSetConstantBuffers(0, _countof(constantBuffers), constantBuffers);

	// シーン用定数バッファ更新
	CbSkyMap cbSkyMap;

	auto mc = scene->GetMainCamera();
	Matrix View = mc->GetView();
	Matrix Projection = mc->GetProjection();
	cbSkyMap.inverseViewProjection = (View * Projection).Invert();

	cbSkyMap.cameraPosition = mc->owner->GetWorldTransform().position;
	dc->UpdateSubresource(skyMapConstantBuffer.Get(), 0, 0, &cbSkyMap, 0, 0);

	// DRAW

	Vector3 dxyz(0, 0, 0);
	Vector2 dwh(graphics.GetScreenWidth(), graphics.GetScreenHeight());
	Vector2 sxy(0, 0);
	Vector2 swh(textureWidth, textureHeight);

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
		v[i].color.x = 1;
		v[i].color.y = 1;
		v[i].color.z = 1;
		v[i].color.w = 1;
		v[i].texcoord.x = texcoords[i].x / textureWidth;
		v[i].texcoord.y = texcoords[i].y / textureHeight;
	}

	// 頂点バッファの内容の編集を終了する。
	dc->Unmap(vertexBuffer.Get(), 0);

	// GPUに描画するためのデータを渡す
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	dc->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);
	dc->IASetInputLayout(inputLayout.Get());
	dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	dc->PSSetShaderResources(0, 1, shaderResourceView.GetAddressOf()); // テクスチャを設定

	// 描画
	dc->Draw(4, 0);
}

void SkyMapRenderer::LoadPanorama(const std::string& path)
{
	texturePath = path;

	HRESULT hr;
	auto device = Graphics::Instance().GetDevice();

	D3D11_TEXTURE2D_DESC desc;

	// テクスチャファイル読み込み
	hr = GpuResourceUtils::LoadTexture(
		device,
		texturePath.empty() ? "sky.dds" : texturePath,
		shaderResourceView.GetAddressOf(),
		&desc);

	_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
	textureWidth = static_cast<float>(desc.Width);
	textureHeight = static_cast<float>(desc.Height);
}

void SkyMapRenderer::OnInspector()
{
	// ボタンを押したらポップアップを開く
	if (ImGui::ImageButton("Texture", (void*)shaderResourceView.Get(), ImVec2(128, 128)))
	{
		ImGui::OpenPopup("SkyMapTexturePopup");
	}

	// ポップアップを開始
	if (ImGui::BeginPopup("SkyMapTexturePopup"))
	{
		// "<none>" 選択肢
		if (ImGui::Selectable("<none>", texturePath.empty()))
		{
			texturePath.clear();
			LoadPanorama("");
			ImGui::CloseCurrentPopup();
		}

		// ddsList の表示
		auto ddsList = AssetManager::Instance().ScanAssets(".dds");
		for (auto& dds : ddsList)
		{
			std::string rel = dds.string();
			bool selected = (rel == texturePath);

			if (ImGui::Selectable(rel.c_str(), selected))
			{
				texturePath = rel;
				LoadPanorama(texturePath);
				ImGui::CloseCurrentPopup();
			}

			// ホバーでプレビュー
			if (ImGui::IsItemHovered())
			{
				ImGui::BeginTooltip();
				ImGui::TextUnformatted(rel.c_str());
				ImGui::Separator();

				static Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
				static std::string cachedPath;

				if (cachedPath != rel)
				{
					srv.Reset();
					if (SUCCEEDED(DirectX::CreateDDSTextureFromFile(
						Graphics::Instance().GetDevice(),
						std::filesystem::path(rel).wstring().c_str(),
						nullptr,
						srv.GetAddressOf())))
					{
						cachedPath = rel;
					}
				}

				if (srv)
				{
					ImGui::Image((ImTextureID)srv.Get(), ImVec2(128, 128));
				}
				else
				{
					ImGui::TextColored(ImVec4(1, 0, 0, 1), "SRV NULL");
				}

				ImGui::EndTooltip();
			}
		}

		ImGui::EndPopup();
	}
}

