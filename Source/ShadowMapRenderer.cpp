#include "Misc.h"
#include "GpuResourceUtils.h"
#include "Scene.h"
#include "ShadowMapRenderer.h"
#include <Graphics.h>
#include "Model.h"

void ShadowMapRenderer::RemakeTextures()
{
	auto device = Graphics::Instance().GetDevice();

	// 深度ステンシルビュー＆シェーダーリソースビューの生成
	{
		// テクスチャ生成
		D3D11_TEXTURE2D_DESC texture2dDesc{};
		texture2dDesc.Width = shadowTexelSize;
		texture2dDesc.Height = shadowTexelSize;
		texture2dDesc.MipLevels = 1;
		texture2dDesc.ArraySize = 1;
		texture2dDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		texture2dDesc.SampleDesc.Count = 1;
		texture2dDesc.SampleDesc.Quality = 0;
		texture2dDesc.Usage = D3D11_USAGE_DEFAULT;
		texture2dDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
		texture2dDesc.CPUAccessFlags = 0;
		texture2dDesc.MiscFlags = 0;
		Microsoft::WRL::ComPtr<ID3D11Texture2D> texture2d;
		HRESULT hr = device->CreateTexture2D(&texture2dDesc, nullptr, texture2d.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));

		// 深度ステンシルビューの生成
		D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc{};
		depthStencilViewDesc.Format = DXGI_FORMAT_D32_FLOAT;
		depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		depthStencilViewDesc.Texture2D.MipSlice = 0;
		hr = device->CreateDepthStencilView(texture2d.Get(), &depthStencilViewDesc,
			depthStencilView.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));

		// シェーダーリソースビューの生成
		D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
		shaderResourceViewDesc.Format = DXGI_FORMAT_R32_FLOAT;
		shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
		shaderResourceViewDesc.Texture2D.MipLevels = 1;
		hr = device->CreateShaderResourceView(texture2d.Get(), &shaderResourceViewDesc,
			shaderResourceView.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
	}
}

// コンストラクタ
ShadowMapRenderer::ShadowMapRenderer(UINT shadowTexelSize)
	: shadowTexelSize(shadowTexelSize)
{
	auto device = Graphics::Instance().GetDevice();

	// 入力レイアウト
	D3D11_INPUT_ELEMENT_DESC inputElementDesc[]
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT },
		{ "WEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT },
		{ "BONES", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT },
	};

	// 頂点シェーダー
	GpuResourceUtils::LoadVertexShader(
		device,
		"ShadowMapVS._cso", inputElementDesc,
		_countof(inputElementDesc),
		inputLayout.GetAddressOf(),
		vertexShader.GetAddressOf());

	// スキニング用定数バッファ
	GpuResourceUtils::CreateConstantBuffer(
		device,
		sizeof(SKINNING_CONSTANT_BUFFER),
		skinningConstantBuffer.GetAddressOf());

	// オブジェクト用定数バッファ
	GpuResourceUtils::CreateConstantBuffer(
		device,
		sizeof(OBJECT_CONSTANT_BUFFER),
		objectConstantBuffer.GetAddressOf());

	// シーン用定数バッファ
	GpuResourceUtils::CreateConstantBuffer(
		device,
		sizeof(SCENE_CONSTANT_BUFFER),
		sceneConstantBuffer.GetAddressOf());

	RemakeTextures();

	// サンプラステート
	{
		D3D11_SAMPLER_DESC desc{};
		desc.MipLODBias = 0.0f;
		desc.MaxAnisotropy = 1;
		desc.ComparisonFunc = D3D11_COMPARISON_LESS;
		desc.MinLOD = 0;
		desc.MaxLOD = 0;
		desc.BorderColor[0] = D3D11_FLOAT32_MAX;
		desc.BorderColor[1] = D3D11_FLOAT32_MAX;
		desc.BorderColor[2] = D3D11_FLOAT32_MAX;
		desc.BorderColor[3] = D3D11_FLOAT32_MAX;
		desc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
		desc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
		desc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
		desc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
		HRESULT hr = device->CreateSamplerState(&desc, samplerState.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
	}
}

// 開始処理
void ShadowMapRenderer::Begin(Scene* scene, const DirectX::XMFLOAT3& position)
{
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();

	// 設定中のレンダーターゲットを一時的に保持する
	UINT numViewports = 1;
	dc->RSGetViewports(&numViewports, &cachedViewport);
	dc->OMGetRenderTargets(1, cachedRenderTargetView.ReleaseAndGetAddressOf(),
		cachedDepthStencilView.ReleaseAndGetAddressOf());

	// レンダーターゲット＆深度ステンシル設定
	dc->OMSetRenderTargets(0, nullptr, depthStencilView.Get());
	dc->ClearDepthStencilView(depthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

	// ビューポート設定
	D3D11_VIEWPORT viewport;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = static_cast<float>(shadowTexelSize);
	viewport.Height = static_cast<float>(shadowTexelSize);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	dc->RSSetViewports(1, &viewport);

	// シェーダー設定
	dc->VSSetShader(vertexShader.Get(), nullptr, 0);
	dc->PSSetShader(nullptr, nullptr, 0);
	dc->IASetInputLayout(inputLayout.Get());

	// レンダーステート設定
	auto rs = scene->GetRenderState();
	const float blendFactor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	dc->OMSetBlendState(rs->GetBlendState(BlendState::Opaque), blendFactor, 0xFFFFFFFF);
	dc->OMSetDepthStencilState(nullptr, 0);
	dc->RSSetState(rs->GetRasterizerState(RasterizerState::SolidCullBack));

	// 定数バッファ設定
	ID3D11Buffer* constantBuffers[] =
	{
		skinningConstantBuffer.Get(),
		objectConstantBuffer.Get(),
		sceneConstantBuffer.Get(),
	};
	dc->VSSetConstantBuffers(0, _countof(constantBuffers), constantBuffers);

	auto directionalLight = scene->GetDirectionalLight();
	if (directionalLight)
	{
		// forward を回す
		Vector3 dir = Vector3::TransformNormal(
			Vector3::Forward,
			Matrix::CreateFromQuaternion(
				directionalLight->owner->GetWorldTransform().rotation));
		dir.Normalize();

		Vector3 Focus = Vector3(0, 0, 0);//scene->mainCamera.GetFocus();
		Vector3 Eye = Focus - dir * 50.0f;
		Matrix View = Matrix::CreateLookAt(Eye, Focus, Vector3::Up);
		float range = 120.0f;
		Matrix Projection = Matrix::CreateOrthographic(range, range, 0.1f, 1000.0f);
		lightViewProjection = View * Projection;
	}
}

// 描画実行
void ShadowMapRenderer::Draw(Scene* scene, const Model* model)
{
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
	for (const Model::Mesh& mesh : model->GetMeshes())
	{
		// 頂点バッファ設定
		UINT stride = sizeof(Model::Vertex);
		UINT offset = 0;
		dc->IASetVertexBuffers(0, 1, mesh.vertexBuffer.GetAddressOf(), &stride, &offset);
		dc->IASetIndexBuffer(mesh.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
		dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// スキニング用定数バッファ更新
		SKINNING_CONSTANT_BUFFER skinningConstantBufferData{};
		if (mesh.bones.size() > 0)
		{
			for (size_t i = 0; i < mesh.bones.size(); ++i)
			{
				const Model::Bone& bone = mesh.bones[i];
				skinningConstantBufferData.boneTransforms[i] = bone.offsetTransform * bone.node->globalTransform;
			}
		}
		else
		{
			skinningConstantBufferData.boneTransforms[0] = mesh.node->globalTransform;
		}
		dc->UpdateSubresource(skinningConstantBuffer.Get(), 0, 0, &skinningConstantBufferData, 0, 0);

		// オブジェクト用定数バッファ更新
		OBJECT_CONSTANT_BUFFER objectConstantBufferData{};
		objectConstantBufferData.world = model->owner->GetWorldTransform().GetMatrix();
		dc->UpdateSubresource(objectConstantBuffer.Get(), 0, 0, &objectConstantBufferData, 0, 0);

		// シーン用定数バッファ更新
		SCENE_CONSTANT_BUFFER sceneConstantBufferData{};
		sceneConstantBufferData.lightViewProjection = lightViewProjection;
		dc->UpdateSubresource(sceneConstantBuffer.Get(), 0, 0, &sceneConstantBufferData, 0, 0);

		// 描画
		dc->DrawIndexed(static_cast<UINT>(mesh.indices.size()), 0, 0);
	}
}

// 終了処理
void ShadowMapRenderer::End()
{
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();

	// レンダーターゲットを元の状態に戻す
	dc->OMSetRenderTargets(1, cachedRenderTargetView.GetAddressOf(), cachedDepthStencilView.Get());
	dc->RSSetViewports(1, &cachedViewport);
}

void ShadowMapRenderer::OnInspector()
{
	ImGui::Text("Shadow Map Renderer");
	if (ImGui::SliderInt("Shadow Texel Size", reinterpret_cast<int*>(&shadowTexelSize), 256, 12288)) {
		RemakeTextures();
	}

	ImGui::Image(
		GetShaderResourceView(),
		ImVec2(128, 128),
		ImVec2(0, 0),
		ImVec2(1, 1),
		ImVec4(1, 1, 1, 1),
		ImVec4(0, 0, 0, 1)
	);
}

void ShadowMapRenderer::Serialize(json& j)
{
	j["shadowTexelSize"] = shadowTexelSize;
}

void ShadowMapRenderer::Deserialize(json& j)
{
	shadowTexelSize = j["shadowTexelSize"];
	RemakeTextures();
}