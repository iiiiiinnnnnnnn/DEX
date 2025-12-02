#include "Misc.h"
#include "GpuResourceUtils.h"
#include "PhongShader.h"

PhongShader::PhongShader(ID3D11Device* device)
{
	// 入力レイアウト
	// 入力レイアウト
	D3D11_INPUT_ELEMENT_DESC inputElementDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BONE_WEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BONE_INDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	// 頂点シェーダー
	GpuResourceUtils::LoadVertexShader(
		device,
		"Data/Shader/PhongVS.cso",
		inputElementDesc,
		_countof(inputElementDesc),
		inputLayout.GetAddressOf(),
		vertexShader.GetAddressOf());

	// ピクセルシェーダー
	GpuResourceUtils::LoadPixelShader(
		device,
		"Data/Shader/PhongPS.cso",
		pixelShader.GetAddressOf());

	// シーン用定数バッファ
	GpuResourceUtils::CreateConstantBuffer(
		device,
		sizeof(CbScene),
		sceneConstantBuffer.GetAddressOf());

	// メッシュ用定数バッファ
	GpuResourceUtils::CreateConstantBuffer(
		device,
		sizeof(CbMesh),
		meshConstantBuffer.GetAddressOf());

	// スケルトン用定数バッファ
	GpuResourceUtils::CreateConstantBuffer(
		device,
		sizeof(CbSkeleton),
		skeletonConstantBuffer.GetAddressOf());
}
// 描画開始
void PhongShader::Begin(const RenderContext& rc)
{
	ID3D11DeviceContext* dc = rc.deviceContext;

	// シェーダー設定
	dc->IASetInputLayout(inputLayout.Get());
	rc.deviceContext->VSSetShader(vertexShader.Get(), nullptr, 0);
	rc.deviceContext->PSSetShader(pixelShader.Get(), nullptr, 0);

	// 定数バッファ設定
	ID3D11Buffer* constantBuffers[] =
	{
		sceneConstantBuffer.Get(),
		meshConstantBuffer.Get(),
		skeletonConstantBuffer.Get(),
	};
	rc.deviceContext->VSSetConstantBuffers(0, _countof(constantBuffers), constantBuffers);
	rc.deviceContext->PSSetConstantBuffers(0, _countof(constantBuffers), constantBuffers);

	// サンプラステート設定
	ID3D11SamplerState* samplerStates[] =
	{
		rc.renderState->GetSamplerState(SamplerState::LinearWrap),
		rc.shadowMap->GetSamplerState(),
	};
	dc->PSSetSamplers(0, _countof(samplerStates), samplerStates);

	// レンダーステート設定
	const float blend_factor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	dc->OMSetBlendState(rc.renderState->GetBlendState(BlendState::Transparency),
		blend_factor, 0xFFFFFFFF);
	dc->OMSetDepthStencilState(rc.renderState->GetDepthStencilState(DepthState::TestAndWrite),
		0);
	dc->RSSetState(rc.renderState->GetRasterizerState(RasterizerState::SolidCullBack));

	// シーン用定数バッファ更新
	CbScene cbScene{};
	DirectX::XMMATRIX V = DirectX::XMLoadFloat4x4(&rc.camera->GetView());
	DirectX::XMMATRIX P = DirectX::XMLoadFloat4x4(&rc.camera->GetProjection());
	DirectX::XMStoreFloat4x4(&cbScene.viewProjection, V * P);
	cbScene.lightViewProjection = rc.shadowMap->GetLightViewProjection();
	cbScene.shadowColor.x = rc.shadowColor.x;
	cbScene.shadowColor.y = rc.shadowColor.y;
	cbScene.shadowColor.z = rc.shadowColor.z;
	cbScene.shadowTexelSize = rc.shadowMap->GetTexelSize();

	const DirectionalLight& directionalLight = rc.lightManager->GetDirectionalLight();
	cbScene.lightDirection.x = directionalLight.direction.x;
	cbScene.lightDirection.y = directionalLight.direction.y;
	cbScene.lightDirection.z = directionalLight.direction.z;
	cbScene.lightColor.x = directionalLight.color.x;
	cbScene.lightColor.y = directionalLight.color.y;
	cbScene.lightColor.z = directionalLight.color.z;

	const DirectX::XMFLOAT3& eye = rc.camera->GetEye();
	cbScene.cameraPosition.x = eye.x;
	cbScene.cameraPosition.y = eye.y;
	cbScene.cameraPosition.z = eye.z;
	dc->UpdateSubresource(sceneConstantBuffer.Get(), 0, 0, &cbScene, 0, 0);
}

// 描画
void PhongShader::Draw(const RenderContext& rc, const Model* model)
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

		// メッシュ用定数バッファ更新
		CbMesh cbMesh{};
		cbMesh.materialColor = mesh.material->color;
		dc->UpdateSubresource(meshConstantBuffer.Get(), 0, 0, &cbMesh, 0, 0);

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

		// シェーダーリソースビュー設定
		dc->PSSetShaderResources(0, 1, mesh.material->diffuseMap.GetAddressOf());

		// シェーダーリソースビュー設定
		ID3D11ShaderResourceView* srvs[] =
		{
			mesh.material->diffuseMap.Get(),
			mesh.material->normalMap.Get(),
			rc.shadowMap->GetShaderResourceView(),
		};
		dc->PSSetShaderResources(0, _countof(srvs), srvs);

		// 描画
		dc->DrawIndexed(static_cast<UINT>(mesh.indices.size()), 0, 0);
	}
}

// 描画終了
void PhongShader::End(const RenderContext& rc)
{
	ID3D11DeviceContext* dc = rc.deviceContext;
	dc->VSSetShader(nullptr, nullptr, 0);
	dc->PSSetShader(nullptr, nullptr, 0);
	dc->IASetInputLayout(nullptr);

	// 設定されているシェーダーリソースを解除
	ID3D11ShaderResourceView* srvs[] = { nullptr, nullptr, nullptr };
	dc->PSSetShaderResources(0, _countof(srvs), srvs);
}