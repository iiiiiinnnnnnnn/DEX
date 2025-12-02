#include "Misc.h"
#include "GpuResourceUtils.h"
#include "RenderContext.h"
#include "ShadowMap.h"

// コンストラクタ
ShadowMap::ShadowMap(ID3D11Device* device)
{
	// 入力レイアウト
	D3D11_INPUT_ELEMENT_DESC inputElementDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BONE_WEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BONE_INDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	// 頂点シェーダー
	GpuResourceUtils::LoadVertexShader(
		device,
		"Data/Shader/ShadowMapVS.cso", inputElementDesc,
		_countof(inputElementDesc),
		inputLayout.GetAddressOf(),
		vertexShader.GetAddressOf());

	// シーン用定数バッファ
	GpuResourceUtils::CreateConstantBuffer(
		device,
		sizeof(CbScene),
		sceneConstantBuffer.GetAddressOf());

	// スケルトン用定数バッファ
	GpuResourceUtils::CreateConstantBuffer(
		device,
		sizeof(CbSkeleton),
		skeletonConstantBuffer.GetAddressOf());

	// 深度ステンシルビュー＆シェーダーリソースビューの生成
	{
		// テクスチャ生成
		D3D11_TEXTURE2D_DESC texture2dDesc{};
		texture2dDesc.Width = textureSize;
		texture2dDesc.Height = textureSize;
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
void ShadowMap::Begin(const RenderContext& rc, const DirectX::XMFLOAT3& position)
{
	ID3D11DeviceContext* dc = rc.deviceContext;

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
	viewport.Width = static_cast<float>(textureSize);
	viewport.Height = static_cast<float>(textureSize);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	dc->RSSetViewports(1, &viewport);

	// シェーダー設定
	dc->VSSetShader(vertexShader.Get(), nullptr, 0);
	dc->PSSetShader(nullptr, nullptr, 0);
	dc->IASetInputLayout(inputLayout.Get());

	// レンダーステート設定
	const float blendFactor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	dc->OMSetBlendState(rc.renderState->GetBlendState(BlendState::Opaque), blendFactor, 0xFFFFFFFF);
	dc->OMSetDepthStencilState(nullptr, 0);
	dc->RSSetState(rc.renderState->GetRasterizerState(RasterizerState::SolidCullBack));

	// 定数バッファ設定
	ID3D11Buffer* constantBuffers[] =
	{
		sceneConstantBuffer.Get(),
		nullptr,
		skeletonConstantBuffer.Get(),
	};
	dc->VSSetConstantBuffers(0, _countof(constantBuffers), constantBuffers);

	// ライトビュープロジェクション行列を作成
	const DirectionalLight& directionalLight = rc.lightManager->GetDirectionalLight();
	DirectX::XMVECTOR LightDirection = DirectX::XMLoadFloat3(&directionalLight.direction);
	LightDirection = DirectX::XMVector3Normalize(LightDirection);
	DirectX::XMVECTOR Up = DirectX::XMVectorSet(0, 1, 0, 0);
	DirectX::XMVECTOR Focus = DirectX::XMLoadFloat3(&position);
	DirectX::XMVECTOR Eye = DirectX::XMVectorSubtract(Focus, DirectX::XMVectorScale(LightDirection, 50.0f));
	DirectX::XMMATRIX View = DirectX::XMMatrixLookAtLH(Eye, Focus, Up);
	DirectX::XMMATRIX Projection = DirectX::XMMatrixOrthographicLH(120, 120, 0.1f, 200.0f);
	DirectX::XMMATRIX ViewProjection = DirectX::XMMatrixMultiply(View, Projection);
	DirectX::XMStoreFloat4x4(&lightViewProjection, ViewProjection);

	// シーン用定数バッファ更新
	CbScene cbScene;
	DirectX::XMStoreFloat4x4(&cbScene.lightViewProjection, ViewProjection);
	dc->UpdateSubresource(sceneConstantBuffer.Get(), 0, 0, &cbScene, 0, 0);
}

// 描画実行
void ShadowMap::Draw(const RenderContext& rc, const Model* model)
{
	ID3D11DeviceContext* dc = rc.deviceContext;
	for (const Model::Mesh& mesh : model->GetMeshes())
	{
		// 頂点バッファ設定
		UINT stride = sizeof(Model::Vertex);
		UINT offset = 0;
		dc->IASetVertexBuffers(0, 1, mesh.vertexBuffer.GetAddressOf(), &stride, &offset);
		dc->IASetIndexBuffer(mesh.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
		dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// スケルトン用定数バッファ更新
		CbSkeleton cbSkeleton{};
		if (mesh.bones.size() > 0)
		{
			for (size_t i = 0; i < mesh.bones.size(); ++i)
			{
				const Model::Bone& bone = mesh.bones.at(i);
				DirectX::XMMATRIX WorldTransform = DirectX::XMLoadFloat4x4(&bone.node->worldTransform);
				DirectX::XMMATRIX OffsetTransform = DirectX::XMLoadFloat4x4(&bone.offsetTransform);
				DirectX::XMMATRIX BoneTransform = OffsetTransform * WorldTransform;
				DirectX::XMStoreFloat4x4(&cbSkeleton.boneTransforms[i], BoneTransform);
			}
		}
		else
		{
			cbSkeleton.boneTransforms[0] = mesh.node->worldTransform;
		}
		rc.deviceContext->UpdateSubresource(skeletonConstantBuffer.Get(), 0, 0, &cbSkeleton, 0, 0);

		// 描画
		dc->DrawIndexed(static_cast<UINT>(mesh.indices.size()), 0, 0);
	}
}

// 終了処理
void ShadowMap::End(const RenderContext& rc)
{
	ID3D11DeviceContext* dc = rc.deviceContext;

	// レンダーターゲットを元の状態に戻す
	dc->OMSetRenderTargets(1, cachedRenderTargetView.GetAddressOf(), cachedDepthStencilView.Get());
	dc->RSSetViewports(1, &cachedViewport);
}