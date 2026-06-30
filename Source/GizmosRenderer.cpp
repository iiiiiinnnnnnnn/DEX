#include "Misc.h"
#include "GpuResourceUtils.h"
#include "GizmosRenderer.h"
#include "Camera.h"
#include <Graphics.h>
#include "Scene.h"

// これさ、カメラのデバッグ表示を作りたいんだけど、どうすればいい？

// コンストラクタ
GizmosRenderer::GizmosRenderer()
{
	auto device = Graphics::Instance().GetDevice();

	// 入力レイアウト
	D3D11_INPUT_ELEMENT_DESC inputElementDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	// 頂点シェーダー
	GpuResourceUtils::LoadVertexShader(
		device,
		"Data/Shader/GizmosVS.cso",
		inputElementDesc,
		_countof(inputElementDesc),
		inputLayout.GetAddressOf(),
		vertexShader.GetAddressOf());

	// ピクセルシェーダー
	GpuResourceUtils::LoadPixelShader(
		device,
		"Data/Shader/GizmosPS.cso",
		pixelShader.GetAddressOf());

	// 定数バッファ
	GpuResourceUtils::CreateConstantBuffer(
		device,
		sizeof(CbMesh),
		constantBuffer.GetAddressOf());

	// 箱メッシュ生成
	CreateBoxMesh(device, 0.5f, 0.5f, 0.5f);

	// 球メッシュ生成
	CreateSphereMesh(device, 1.0f, 32);

	// ラインメッシュ生成
	{
		std::vector<Vector3> vertices =
		{
			Vector3(0,0,0),
			Vector3(1,0,0)
		};

		CreateMesh(device, vertices, lineMesh);
	}
}

// 箱描画
void GizmosRenderer::DrawBox(
	const Vector3& position,
	const Quaternion& rotation,
	const Vector3& size,
	const Color& color)
{
	Instance& instance = instances.emplace_back();
	instance.mesh = &boxMesh;
	instance.color = color;
	instance.worldTransform =
		Matrix::CreateScale(size)*
		Matrix::CreateFromQuaternion(rotation)*
		Matrix::CreateTranslation(position);
}

// 球描画
void GizmosRenderer::DrawSphere(
	const Vector3& position,
	float radius,
	const Color& color)
{
	Instance& instance = instances.emplace_back();
	instance.mesh = &sphereMesh;
	instance.color = color;
	instance.worldTransform =
		Matrix::CreateScale(radius) *
		Matrix::CreateTranslation(position);
}

// 線描画
void GizmosRenderer::DrawLine(
	const Vector3& a,
	const Vector3& b,
	const Color& color)
{
	Instance& instance = instances.emplace_back();

	instance.mesh = &lineMesh;
	instance.color = color;

	Vector3 dir = b - a;
	float len = dir.Length();
	dir.Normalize();

	Matrix S = Matrix::CreateScale(len, 1, 1);
	Matrix R = Matrix::CreateWorld(a, dir, Vector3::Up);

	instance.worldTransform = S * R;
}

// メッシュ生成
void GizmosRenderer::CreateMesh(ID3D11Device* device, const std::vector<Vector3>& vertices, Mesh& mesh)
{
	D3D11_BUFFER_DESC desc = {};
	desc.ByteWidth = static_cast<UINT>(sizeof(Vector3) * vertices.size());
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	desc.StructureByteStride = 0;
	D3D11_SUBRESOURCE_DATA subresourceData = {};
		subresourceData.pSysMem = vertices.data();
	subresourceData.SysMemPitch = 0;
	subresourceData.SysMemSlicePitch = 0;
	HRESULT hr = device->CreateBuffer(&desc, &subresourceData, mesh.vertexBuffer.GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
	mesh.vertexCount = static_cast<UINT>(vertices.size());
}

// 箱メッシュ作成
void GizmosRenderer::CreateBoxMesh(ID3D11Device* device, float width, float height, float depth)
{
	Vector3 positions[8] =
	{
		// top
		{ -width, height, -depth},
		{ width, height, -depth},
		{ width, height, depth},
		{ -width, height, depth},
		// bottom
		{ -width, -height, -depth},
		{ width, -height, -depth},
		{ width, -height, depth},
		{ -width, -height, depth},
	};
	std::vector<Vector3> vertices;
	vertices.resize(32); // reserve?
	// top
	vertices.emplace_back(positions[0]);
	vertices.emplace_back(positions[1]);
	vertices.emplace_back(positions[1]);
	vertices.emplace_back(positions[2]);
	vertices.emplace_back(positions[2]);
	vertices.emplace_back(positions[3]);
	vertices.emplace_back(positions[3]);
	vertices.emplace_back(positions[0]);
	// bottom
	vertices.emplace_back(positions[4]);
	vertices.emplace_back(positions[5]);
	vertices.emplace_back(positions[5]);
	vertices.emplace_back(positions[6]);
	vertices.emplace_back(positions[6]);
	vertices.emplace_back(positions[7]);
	vertices.emplace_back(positions[7]);
	vertices.emplace_back(positions[4]);
	// side
	vertices.emplace_back(positions[0]);
	vertices.emplace_back(positions[4]);
	vertices.emplace_back(positions[1]);
	vertices.emplace_back(positions[5]);
	vertices.emplace_back(positions[2]);
	vertices.emplace_back(positions[6]);
	vertices.emplace_back(positions[3]);
	vertices.emplace_back(positions[7]);

	// メッシュ生成
	CreateMesh(device, vertices, boxMesh);
}

// 球メッシュ作成
void GizmosRenderer::CreateSphereMesh(ID3D11Device* device, float radius, int subdivisions)
{
	float step = DirectX::XM_2PI / subdivisions;
	std::vector<Vector3> vertices;
	// XZ平面
	for (int i = 0; i < subdivisions; ++i)
	{
		for (int j = 0; j < 2; ++j)
		{
			float theta = step * ((i + j) % subdivisions);
			Vector3& p = vertices.emplace_back();
			p.x = sinf(theta) * radius;
			p.y = 0.0f;
			p.z = cosf(theta) * radius;
		}
	}
	// XY平面
	for (int i = 0; i < subdivisions; ++i)
	{
		for (int j = 0; j < 2; ++j)
		{
			float theta = step * ((i + j) % subdivisions);
			Vector3& p = vertices.emplace_back();
			p.x = sinf(theta) * radius;
			p.y = cosf(theta) * radius;
			p.z = 0.0f;
		}
	}
	// YZ平面
	for (int i = 0; i < subdivisions; ++i)
	{
		for (int j = 0; j < 2; ++j)
		{
			float theta = step * ((i + j) % subdivisions);
			Vector3& p = vertices.emplace_back();
			p.x = 0.0f;
			p.y = sinf(theta) * radius;
			p.z = cosf(theta) * radius;
		}
	}
	// メッシュ生成
	CreateMesh(device, vertices, sphereMesh);
}

// 描画実行
void GizmosRenderer::Render(Scene* scene)
{
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();

	// シェーダー設定
	dc->VSSetShader(vertexShader.Get(), nullptr, 0);
	dc->PSSetShader(pixelShader.Get(), nullptr, 0);
	dc->IASetInputLayout(inputLayout.Get());

	// 定数バッファ設定
	// b0 のスロットに定数バッファを渡す
	dc->VSSetConstantBuffers(0, 1, constantBuffer.GetAddressOf());

	// レンダーステート設定
	const float blendFactor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	auto rs = scene->GetRenderState();
	dc->OMSetBlendState(rs->GetBlendState(BlendState::Opaque), blendFactor, 0xFFFFFFFF);
	dc->OMSetDepthStencilState(rs->GetDepthStencilState(DepthState::TestAndWrite), 0);
	dc->RSSetState(rs->GetRasterizerState(RasterizerState::SolidCullNone));

	// ビュープロジェクション行列作成
	auto mc = scene->GetMainCamera();
	Matrix VP = mc->GetView() * mc->GetProjection();

	// プリミティブ設定
	UINT stride = sizeof(Vector3);
	UINT offset = 0;
	dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST); // LineListで描画
	for (const Instance& instance : instances)
	{
		// 頂点バッファ設定
		dc->IASetVertexBuffers(0, 1, instance.mesh->vertexBuffer.GetAddressOf(), &stride, &offset);

		// ワールドビュープロジェクション行列作成
		Matrix WVP = instance.worldTransform * VP;

		// 定数バッファ更新
		CbMesh cbMesh;
		cbMesh.worldViewProjection = WVP;
		cbMesh.color = instance.color;
		dc->UpdateSubresource(constantBuffer.Get(), 0, 0, &cbMesh, 0, 0);

		// 描画
		dc->Draw(instance.mesh->vertexCount, 0);
	}
	instances.clear();
}